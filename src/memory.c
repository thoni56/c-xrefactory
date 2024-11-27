#include "memory.h"

#include <stdlib.h>

#include "log.h"
#include "head.h"
#include "constants.h"
#include "proto.h"


jmp_buf memoryResizeJumpTarget;


/* Dynamic memory */
Memory *cxMemory=NULL;

/* Static memory areas */
Memory2 ppmMemory;


/* This is used unless the fatalError function is set */
static void defaultFatalMemoryErrorHandler(int errorCode, char *message, int exitStatus, char *file, int line) {
    log_fatal("Error code: %d, Message: '%s' in file %s", errorCode, message, file);
    exit(exitStatus);
}

/* Inject the function to call when fatalErrors occur */
static void (*fatalMemoryError)(int errCode, char *mess, int exitStatus, char *file, int line) = defaultFatalMemoryErrorHandler;
void setFatalErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus, char *file,
                                                    int line)) {
    fatalMemoryError = function;
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

void dm_init(Memory *memory, char *name) {
    ENTER();
    memory->name = name;
    memory->index = 0;
    //memory->overflowHandler = NULL;
    LEAVE();
}


void *dm_allocc(Memory *memory, int count, size_t size) {
    int previous_index;

    assert(count >= 0);
    if (memory->index+count*size >= memory->size) {
        if (memory->overflowHandler != NULL && memory->overflowHandler(count))
            memoryResized();
        else
            fatalMemoryError(ERR_NO_MEMORY, memory->name, XREF_EXIT_ERR, __FILE__, __LINE__);
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
void *olcxAlloc(size_t size) {
    return malloc(size);
}


void olcxFree(void *pointer, size_t size) {
    free(pointer);
}

/* EDITOR */
void *editorAlloc(size_t size) {
    return olcxAlloc(size);
}

void editorFree(void *pointer, size_t size) {
    olcxFree(pointer, size);
}

static bool isInMemory(Memory2 *memory, void *pointer) {
    return pointer >= (void *)memory->area && pointer <= (void *)&memory->area[memory->size];
}

void smInit(Memory2 *memory, char *name, size_t size) {
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
    memory->name = name;
}

void *smAllocc(Memory2 *memory, int count, size_t size) {
    void *pointer = &memory->area[memory->index];
    assert(size > 0);
    assert(count >= 0);
    memory->index += count*size;
    if (memory->index > memory->size) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "%s memory overflow", memory->name);
        fatalMemoryError(ERR_ST, tmpBuff, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
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
    assert(isInMemory(memory, pointer));
    memory->index = (char *)pointer - memory->area;
}

bool smIsBetween(Memory2 *memory, void *pointer, int low, int high) {
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
