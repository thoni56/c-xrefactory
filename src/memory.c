#include "memory.h"

#include "log.h"

#include "constants.h"
#include "proto.h"


static bool memoryTrace = false;
#define mem_trace(...)  { if (memoryTrace) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__); }


jmp_buf memoryResizeJumpTarget;

int olcxMemoryAllocatedBytes;

CodeBlock *currentBlock;


/* Dynamic memory */
Memory *cxMemory=NULL;

/* Stack memory - in this we allocate blocks which have separate free indices */
char stackMemory[SIZE_stackMemory];   /* Allocation using stackMemoryAlloc() et.al */

/* Static memory areas */
Memory2 ppmMemory;


/* This is used unless the fatalError function is set */
static void fallBackFatalError(int errorCode, char *message, int exitStatus, char *file, int line) {
    log_fatal("Error code: %d, Message: '%s' in file %s", errorCode, message, file);
    exit(exitStatus);
}

/* Inject the function to call when fatalErrors occur */
static void (*fatalError)(int errCode, char *mess, int exitStatus, char *file, int line) = fallBackFatalError;
void setFatalErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus, char *file,
                                                    int line)) {
    fatalError = function;
}

/* Copy of a few defines from commons.h to avoid dependency on other stuff... */
#undef assert
#define assert(expr)                                                                                    \
    if (!(expr))                                                                                        \
        internalCheckFailForMemory(#expr, __FILE__, __LINE__)

/* Inject the function to call when assert() fails, a.k.a internalCheckFail() */
static void (*internalCheckFailForMemory)(char *expr, char *file, int line);
void setInternalCheckFailHandlerForMemory(void (*function)(char *expr, char *file, int line)) {
    internalCheckFailForMemory = function;
}

/* Inject the function to call for error() */
static void (*error)(int code, char *message);
void setErrorHandlerForMemory(void (*function)(int code, char *message)) {
    error = function;
}

/* With this as a separate function it is possible to catch memory resize longjmps */
void memoryResized(void) {
    longjmp(memoryResizeJumpTarget,1);
}

void initMemory(Memory *memory, char *name, bool (*overflowHandler)(int n), int size) {
    ENTER();
    memory->name = name;
    memory->overflowHandler = overflowHandler;
    memory->index = 0;
    memory->size = size;
    memory->block[0] = 0;
    LEAVE();
}

/* ************************** Overflow Handlers ************************* */

bool cxMemoryOverflowHandler(int n) {
    int oldfactor, factor, oldsize, newsize;
    Memory *oldcxMemory;

    log_trace("Handling CX memory overflow with n=%d", n);
    if (cxMemory!=NULL) {
        oldsize = cxMemory->size;
    } else {
        oldsize = 0;
    }

    oldfactor = oldsize / CX_MEMORY_CHUNK_SIZE;
    factor = ((n>1)?(n-1):0)/CX_MEMORY_CHUNK_SIZE + 1; // 1 no patience to wait // TODO: WTF?
    //& if (options.cxMemoryFactor>=1) factor *= options.cxMemoryFactor;
    factor += oldfactor;
    if (oldfactor*2 > factor)
        factor = oldfactor*2;
    newsize = factor * CX_MEMORY_CHUNK_SIZE;
    oldcxMemory = cxMemory;
    if (oldcxMemory!=NULL)
        free(oldcxMemory);
    cxMemory = malloc(newsize + sizeof(Memory));
    if (cxMemory!=NULL) {
        initMemory(cxMemory, "", cxMemoryOverflowHandler, newsize);
    }
    log_debug("Reallocating cxMemory: %d -> %d", oldsize, newsize);

    return cxMemory != NULL;
}

/* ***************************************************************** */

static void trailDump(void) {
    log_trace("*** begin trailDump");
    for (FreeTrail *t=currentBlock->trail; t!=NULL; t=t->next)
        log_trace("%p ", t);
    log_trace("*** end trailDump");
}


void addToTrail(void (*action)(void*), void *pointer, bool needTrailOnTopLevel) {
    FreeTrail *t;

    /* no trail at level 0 in C, Yacc */
    if ((nestingLevel() == 0) && !needTrailOnTopLevel)
        return;
    t = StackMemoryAlloc(FreeTrail);
    t->action = action;
    t->pointer = (void **) pointer;
    t->next = currentBlock->trail;
    currentBlock->trail = t;
    if (memoryTrace)
        trailDump();
}

void removeFromTrailUntil(FreeTrail *untilP) {
    FreeTrail *p;
    for (p=currentBlock->trail; untilP<p; p=p->next) {
        assert(p!=NULL);
        (*(p->action))(p->pointer);
    }
    if (p!=untilP) {
        error(ERR_INTERNAL, "block structure mismatch?");
    }
    currentBlock->trail = p;
    if (memoryTrace)
        trailDump();
}

static void fillCodeBlock(CodeBlock *block, int firstFreeIndex, FreeTrail *trail, CodeBlock *outerBlock) {
    block->firstFreeIndex = firstFreeIndex;
    block->trail = trail;
    block->outerBlock = outerBlock;
}

void initOuterCodeBlock(void) {
    /* Any use of stack memory will require that this is run first */
    currentBlock = (CodeBlock *) stackMemory;
    fillCodeBlock(currentBlock, sizeof(CodeBlock), NULL, NULL);
}

void *stackMemoryAlloc(int size) {
    int i;

    assert(currentBlock);
    mem_trace("stackMemoryAlloc: allocating %d bytes", size);
    i = currentBlock->firstFreeIndex;
    i = ((char *)ALIGNMENT(stackMemory+i,STANDARD_ALIGNMENT))-stackMemory;
    if (i+size < SIZE_stackMemory) {
        currentBlock->firstFreeIndex = i+size;
        return &stackMemory[i];
    } else {
        fatalError(ERR_ST,"i+size > SIZE_stackMemory,\n\tstack memory overflowed,\n\tread TROUBLES section of README file\n", XREF_EXIT_ERR, __FILE__, __LINE__);
        /* Should not return, but for testing and compilers sake return something */
        return NULL;
    }
}

static void *stackMemoryPush(void *pointer, int size) {
    void *new = stackMemoryAlloc(size);
    memcpy(new, pointer, size);
    return new;
}

char *stackMemoryPushString(char *string) {
    log_trace("Pushing string '%s'", string);
    return (char*)stackMemoryPush(string, strlen(string)+1);
}

void beginBlock(void) {
    CodeBlock *pushed, previous;
    log_trace("Begin block");
    previous = *currentBlock;
    pushed = stackMemoryPush(&previous, sizeof(CodeBlock));
    // trail can't be reset to NULL, because in case of syntax errors
    // this would avoid balancing of } at the end of class
    fillCodeBlock(currentBlock, currentBlock->firstFreeIndex, currentBlock->trail, pushed);
}

void endBlock(void) {
    log_trace("End block");
    //&removeFromTrailUntil(NULL);
    assert(currentBlock && currentBlock->outerBlock);
    removeFromTrailUntil(currentBlock->outerBlock->trail);
    *currentBlock =  *currentBlock->outerBlock;
    assert(currentBlock != NULL);
}


int nestingLevel(void) {
    int level = 0;
    CodeBlock *block = currentBlock;
    while (block->outerBlock != NULL) {
        block = block->outerBlock;
        level++;
    }
    return level;
}


bool isMemoryFromPreviousBlock(void *address) {
    return currentBlock->outerBlock != NULL &&
        (char*)address > stackMemory &&
        (char*)address < stackMemory + currentBlock->outerBlock->firstFreeIndex;
}


bool isFreedPointer(void *ptr) {
    return ((char*)ptr >= stackMemory + currentBlock->firstFreeIndex &&
            (char*)ptr < stackMemory + SIZE_stackMemory);
}

void dm_init(Memory *memory, char *name) {
    ENTER();
    memory->name = name;
    memory->index = 0;
    //memory->overflowHandler = NULL;
    LEAVE();
}


static void align(Memory *memory) {
    memory->index = ((char*)ALIGNMENT(((char*)&memory->block)+memory->index,STANDARD_ALIGNMENT)) - ((char*)&memory->block);
}


void *dm_allocc(Memory *memory, int count, size_t size) {
    int previous_index;

    assert(count >= 0);
    align(memory);
    if (memory->index+count*size >= memory->size) {
        if (memory->overflowHandler != NULL && memory->overflowHandler(count))
            memoryResized();
        else
            fatalError(ERR_NO_MEMORY, memory->name, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    previous_index = memory->index;
    memory->index += (count)*size;
    return (void *) (((char*)&memory->block) + previous_index);
}

void *dm_alloc(Memory *memory, size_t size) {
    return dm_allocc(memory, 1, size);
}
bool dm_enoughSpaceFor(Memory *memory, size_t bytes) {
    return memory->index + bytes < memory->size;
}

bool dm_isBetween(Memory *memory, void *pointer, int low, int high) {
    return pointer >= (void *)&memory->block + low && pointer < (void *)&memory->block + high;
}

bool dm_isFreedPointer(Memory *memory, void *pointer) {
    return dm_isBetween(memory, pointer, memory->index, memory->size);
}

void dm_freeUntil(Memory *memory, void *pointer) {
    assert(pointer >= (void *)&memory->block && pointer <= (void *)&memory->block+memory->index);
    memory->index = (void *)pointer - (void *)&memory->block;
}

/* CX */
void *cxAlloc(size_t size) {
    return dm_alloc(cxMemory, size);
}

void cxFreeUntil(void *until) {
    dm_freeUntil(cxMemory, until);
}

bool isFreedCxMemory(void *pointer) {
    return dm_isFreedPointer(cxMemory, pointer);
}

/* OLCX */
void olcx_memory_init() {
    olcxMemoryAllocatedBytes = 0;
}

void *olcx_memory_soft_allocc(int count, size_t elementSize) {
    int size = count*elementSize;
    if (size+olcxMemoryAllocatedBytes > SIZE_olcxMemory) {
        return NULL;
    } else {
        olcxMemoryAllocatedBytes += size;
        return malloc(size);
    }
}

void *olcx_memory_allocc(int count, size_t elementSize) {
    void *pointer = olcx_memory_soft_allocc(count, elementSize);
    if (pointer==NULL) {
        fatalError(ERR_ST, "olcxMemory memory overflow, please try again.", XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    return pointer;
}

void *olcx_alloc(size_t size) {
    return olcx_memory_allocc(1, size);
}


void olcx_memory_free(void *pointer, size_t size) {
    olcxMemoryAllocatedBytes -= size;
    free(pointer);
}

/* EDITOR */
void *editorAlloc(size_t size) {
    return olcx_memory_allocc(1, size);
}

void editorFree(void *pointer, size_t size) {
    olcx_memory_free(pointer, size);
}

static bool isInMemory(void *pointer, Memory2 *memory) {
    return pointer >= (void *)memory->area && pointer <= (void *)&memory->area[memory->size];
}

void smInit(Memory2 *memory, size_t size) {
    if (size != memory->size) {
        free(memory->area);
        memory->area = NULL;
        memory->size = 0;
    }
    if (memory->area == NULL) {
        memory->area = malloc(size);
        memory->size = size;
    }
    memory->index = 0;
}

void *smAllocc(Memory2 *memory, int count, size_t size) {
    void *pointer = &memory->area[memory->index];
    assert(size > 0);
    assert(count >= 0);
    memory->index += count*size;
    if (memory->index > memory->size)
        fatalError(ERR_ST, "Memory overflow.", XREF_EXIT_ERR, __FILE__, __LINE__);
    return pointer;
}

void *smAlloc(Memory2 *memory, size_t size) {
    return smAllocc(memory, 1, size);
}

/* Reallocates the most recently allocated area in 'memory' to be different size */
void *smRealloc(Memory2 *memory, void *pointer, size_t oldSize, size_t newSize) {
    assert(pointer == &memory->area[memory->index-oldSize]);
    memory->index += newSize - oldSize;
    return pointer;
}

void *smReallocc(Memory2 *memory, void *pointer, int newCount, size_t size, int oldCount) {
    return smRealloc(memory, pointer, oldCount*size, newCount*size);
}


void smFreeUntil(Memory2 *memory, void *pointer) {
    assert(isInMemory(pointer, memory));
    memory->index = (char *)pointer - memory->area;
}

static bool smIsBetween(Memory2 *memory, void *pointer, int low, int high) {
    return pointer >= (void *)&memory->area[low] && pointer < (void *)&memory->area[high];
}

bool smIsFreedPointer(Memory2 *memory, void *pointer) {
    return smIsBetween(memory, pointer, memory->index, memory->size);
}

/* Preprocessor Macro Memory */
void *ppmAlloc(size_t size) {
    return smAlloc(&ppmMemory, size);
}

void *ppmAllocc(int count, size_t size) {
    return smAllocc(&ppmMemory, count, size);
}

void *ppmReallocc(void *pointer, int newCount, size_t size, int oldCount) {
    return smReallocc(&ppmMemory, pointer, newCount, size, oldCount);
}

void ppmFreeUntil(void *pointer) {
    smFreeUntil(&ppmMemory, pointer);
}

bool ppmIsFreedPointer(void *pointer) {
    return smIsFreedPointer(&ppmMemory, pointer);
}
