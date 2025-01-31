#include "memory.h"

#include <stdlib.h>

#include "log.h"
#include "head.h"
#include "constants.h"
#include "proto.h"


jmp_buf memoryResizeJumpTarget;


/* Dynamic memory */
#ifdef USE_NEW_CXMEMORY
FlushableMemory cxMemory;
#else
Memory cxMemory={};
#endif

/* Static memory areas */
Memory ppmMemory;


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
void memoryResized(Memory *memory) {
    log_trace("Memory '%s' has been resized", memory->name);
    longjmp(memoryResizeJumpTarget,1);
}


/* *****************************************************************

   Memory - this new memory type has a dynamic area allocated
            separately from the Memory struct

 */

void memoryInit(Memory *memory, char *name, bool (*overflowHandler)(int n), int size) {
    ENTER();
    memory->name = name;
    memory->overflowHandler = overflowHandler;
    memory->name = name;
    if (size > memory->size) {
        memory->area = realloc(memory->area, size);
        memory->size = size;
    }
    memory->index = 0;
    LEAVE();
}

void *memoryAllocc(Memory *memory, int count, size_t size) {
    void *pointer = &memory->area[memory->index];
    assert(size > 0);
    assert(count >= 0);

    if (memory->index+count*size > memory->size) {
        if (memory->overflowHandler != NULL && memory->overflowHandler(count))
            memoryResized(memory);
        else
            fatalMemoryError(ERR_NO_MEMORY, memory->name, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    memory->index += count*size;
    return pointer;
}

void *memoryAlloc(Memory *memory, size_t size) {
    return memoryAllocc(memory, 1, size);
}

#ifndef USE_NEW_CXMEMORY
static bool memoryHasEnoughSpaceFor(Memory *memory, size_t bytes) {
    return memory->index + bytes < memory->size;
}
#endif

bool memoryIsBetween(Memory *memory, void *pointer, int low, int high) {
    return pointer >= (void *)&memory->area[low] && pointer <= (void *)&memory->area[high];
}

static bool isInMemory(Memory *memory, void *pointer) {
    return memoryIsBetween(memory, pointer, 0, memory->index);
}

void memoryFreeUntil(Memory *memory, void *pointer) {
    assert(isInMemory(memory, pointer));
    memory->index = (char *)pointer - (char *)memory->area;
}

static bool memoryPointerIsFreed(Memory *memory, void *pointer) {
    return memoryIsBetween(memory, pointer, memory->index, memory->size);
}

/* Reallocates the most recently allocated area in 'memory' to be different size */
void *memoryRealloc(Memory *memory, void *pointer, size_t oldSize, size_t newSize) {
    assert(pointer == &memory->area[memory->index-oldSize]);
    memory->index += newSize - oldSize;
    return pointer;
}

// Used by ppmReallocc()
static void *memoryReallocc(Memory *memory, void *pointer, int newCount, size_t size, int oldCount) {
    return memoryRealloc(memory, pointer, oldCount*size, newCount*size);
}

/* Preprocessor Macro Memory */
void *ppmAlloc(size_t size) {
    return memoryAlloc(&ppmMemory, size);
}

void *ppmAllocc(int count, size_t size) {
    return memoryAllocc(&ppmMemory, count, size);
}

void *ppmReallocc(void *pointer, int newCount, size_t size, int oldCount) {
    return memoryReallocc(&ppmMemory, pointer, newCount, size, oldCount);
}

void ppmFreeUntil(void *pointer) {
    memoryFreeUntil(&ppmMemory, pointer);
}

bool ppmIsFreedPointer(void *pointer) {
    return memoryPointerIsFreed(&ppmMemory, pointer);
}


/* CX */
#ifdef USE_NEW_CXMEMORY

void initCxMemory(void) {
    initFlushableMemory(&cxMemory);
}

bool cxMemoryHasEnoughSpaceFor(size_t bytes) {
    return true;
}

bool cxMemoryOverflowHandler(int n) {
    return true;
}

void *cxAlloc(size_t size) {
    return allocateFlushableMemory(&cxMemory, size);
}

bool cxMemoryPointerIsBetween(void *pointer, int low, int high) {
    for (int i=low; i<high; i++) {
        if (cxMemory.blocks[i] == pointer)
            return true;
    }
    return false;
}

bool cxMemoryIsFreed(void *pointer) {
    return flushableMemoryIsFreed(&cxMemory, pointer);
}

void cxFreeUntil(void *pointer) {
    freeFlushableMemoryUntil(&cxMemory, pointer);
}

#else
static int calculateNewSize(int n, int oldsize) {
    int oldfactor, factor, newsize;
    oldfactor = oldsize / CX_MEMORY_CHUNK_SIZE;
    factor = ((n > 1) ? (n - 1) : 0) / CX_MEMORY_CHUNK_SIZE + 1; // 1 no patience to wait // TODO: WTF?
    //& if (options.cxMemoryFactor>=1) factor *= options.cxMemoryFactor;
    factor += oldfactor;
    if (oldfactor * 2 > factor)
        factor = oldfactor * 2;
    newsize = factor * CX_MEMORY_CHUNK_SIZE;

    return newsize;
}

bool cxMemoryHasEnoughSpaceFor(size_t bytes) {
    return memoryHasEnoughSpaceFor(&cxMemory, bytes);
}

bool cxMemoryOverflowHandler(int n) {
    log_trace("Handling CX memory overflow with n=%d", n);
    int oldsize = cxMemory.size;
    int newsize = calculateNewSize(n, oldsize);

    memoryInit(&cxMemory, "cxMemory", cxMemoryOverflowHandler, newsize);
    log_debug("Reallocating cxMemory: %d -> %d", oldsize, newsize);

    return cxMemory.area != NULL;
}

void initCxMemory(void) {
    memoryInit(&cxMemory, "cxMemory", cxMemoryOverflowHandler, CX_MEMORY_CHUNK_SIZE);
}

void *cxAlloc(size_t size) {

    if (cxMemory.index+size >= cxMemory.size) {
        if (cxMemory.overflowHandler != NULL && cxMemory.overflowHandler(size))
            memoryResized(&cxMemory);
        else
            fatalMemoryError(ERR_NO_MEMORY, cxMemory.name, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    int previous_index = cxMemory.index;
    cxMemory.index += size;

    return (void *) &cxMemory.area[previous_index];
}

bool cxMemoryPointerIsBetween(void *pointer, int low, int high) {
    return memoryIsBetween(&cxMemory, pointer, low, high);
}

bool cxMemoryIsFreed(void *pointer) {
    return memoryIsBetween(&cxMemory, pointer, cxMemory.index, cxMemory.size);
}

void cxFreeUntil(void *pointer) {
    memoryFreeUntil(&cxMemory, pointer);
}
#endif

/***********************************************************************/
/* New FlushableMemory */

void initFlushableMemory(FlushableMemory *memory) {
    memory->size = 0;
    memory->top = 0;
    memory->blocks = NULL;
    memory->pendingFlushIndex = -1;
}

void *allocateFlushableMemory(FlushableMemory *memory, size_t size) {
    if (memory->blocks == NULL) {
        memory->blocks = calloc(100, sizeof(void *));
        memory->size = 100;
    }
    if (memory->top == memory->size) {
        memory->blocks = realloc(memory->blocks, memory->size*2*sizeof(memory->blocks[0]));
        memory->size *= 2;
    }

    memory->blocks[memory->top] = malloc(size);
    memory->top++;
    return memory->blocks[memory->top-1];
}

void freeFlushableMemoryUntil(FlushableMemory *memory, void *pointer) {
    assert(memory->size > 0);
    while (memory->top >= 0 && memory->blocks[memory->top] != pointer) {
        free(memory->blocks[memory->top--]);
    }
    assert(memory->top >= 0);
}

bool flushableMemoryIsFreed(FlushableMemory *memory, void *pointer) {
    for (int i = memory->top; i < memory->size; i++)
        if (memory->blocks[i] == pointer)
            return true;
    return false;
}

void markAsFlushable(FlushableMemory *memory, void *pointer) {
    for (int i=memory->top; i >= 0; i--) {
        if (memory->blocks[i] == pointer) {
            memory->pendingFlushIndex = i;
            return;
        }
    }
    assert(0);
}

bool memoryWouldBeFlushed(FlushableMemory *memory, void *pointer) {
    if (memory->pendingFlushIndex == -1)
        return false;           /* No flush pending */

    for (int i=0; i < memory->pendingFlushIndex && i <= memory->top; i++)
        if (memory->blocks[i] == pointer)
            return false;
    return true;
}

void flushPendingMemory(FlushableMemory *memory) {
    for (int i=memory->pendingFlushIndex; i <= memory->top; i++) {
        free(memory->blocks[i]);
        memory->blocks[i] = NULL;
    }
}
