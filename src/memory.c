#include "globals.h"            /* for s_language in addToTrail() */

#include "memory.h"
#include "log.h"


static bool memoryTrace = false;
#define mem_trace(...)  { if (memoryTrace) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__); }


CodeBlock *currentBlock;

Memory *cxMemory=NULL;
int olcxMemoryAllocatedBytes;

jmp_buf memoryResizeJumpTarget;

/* Memory types */
char workMemory[SIZE_workMemory];   /* Allocation using stackMemoryAlloc() et.al */

char ftMemory[SIZE_ftMemory];
int ftMemoryIndex = 0;

char tmpMemory[SIZE_TMP_MEM];

char ppmMemory[SIZE_ppmMemory];
int ppmMemoryIndex=0;

char mbMemory[SIZE_mbMemory];
int mbMemoryIndex=0;



/* Inject the function to call when fatalErrors occur */
static void (*fatalError)(int errCode, char *mess, int exitStatus);
void memoryUseFunctionForFatalError(void (*function)(int errCode, char *mess, int exitStatus)) {
    fatalError = function;
}

/* Inject the function to call when assert() fails, a.k.a internalCheckFail() */
static void (*internalCheckFail)(char *expr, char *file, int line);
void memoryUseFunctionForInternalCheckFail(void (*function)(char *expr, char *file, int line)) {
    internalCheckFail = function;
}

/* Inject the function to call for error() */
static void (*error)(int code, char *message);
void memoryUseFunctionForError(void (*function)(int code, char *message)) {
    error = function;
}


/* With this as a separate function it is possible to catch memory resize longjmps */
void memoryResized(void) {
    longjmp(memoryResizeJumpTarget,1);
}

void initMemory(Memory *memory, bool (*overflowHandler)(int n), int size) {
    memory->overflowHandler = overflowHandler;
    memory->index = 0;
    memory->size = size;
    memory->block = 0;
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
    factor = ((n>1)?(n-1):0)/CX_MEMORY_CHUNK_SIZE + 1; // 1 no patience to wait ;
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
        initMemory(cxMemory, cxMemoryOverflowHandler, newsize);
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


void addToTrail(void (*action)(void*), void *pointer) {
    FreeTrail *t;
    /* no trail at level 0 in C */
    if ((nestingLevel() == 0) && (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)))
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

void stackMemoryInit(void) {
    currentBlock = (CodeBlock *) workMemory;
    fillCodeBlock(currentBlock, sizeof(CodeBlock), NULL, NULL);
}

void *stackMemoryAlloc(int size) {
    int i;

    mem_trace("stackMemoryAlloc: allocating %d bytes", size);
    i = currentBlock->firstFreeIndex;
    i = ((char *)ALIGNMENT(workMemory+i,STANDARD_ALIGNMENT))-workMemory;
    if (i+size < SIZE_workMemory) {
        currentBlock->firstFreeIndex = i+size;
        return &workMemory[i];
    } else {
        fatalError(ERR_ST,"i+size > SIZE_workMemory,\n\tworking memory overflowed,\n\tread TROUBLES section of README file\n", XREF_EXIT_ERR);
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
        (char*)address > workMemory &&
        (char*)address < workMemory + currentBlock->outerBlock->firstFreeIndex;
}


bool freedPointer(void *ptr) {
    return ((char*)ptr >= workMemory + currentBlock->firstFreeIndex &&
            (char*)ptr < workMemory + SIZE_workMemory);
}

void dm_init(Memory *memory, char *name) {
    memory->name = name;
    memory->index = 0;
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
            fatalError(ERR_NO_MEMORY, memory->name, XREF_EXIT_ERR);
    }
    previous_index = memory->index;
    memory->index += (count)*size;
    return (void *) (((char*)&memory->block) + previous_index);
}
