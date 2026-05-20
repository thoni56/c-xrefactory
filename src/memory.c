#include "memory.h"

#include <stdlib.h>

#include "log.h"
#include "head.h"
#include "constants.h"
#include "proto.h"



/* Dynamic memory for cross-references and similar stuff */
Memory cxMemory={};

/* Preprocessor memory */
Memory ppmMemory;


/* This is used unless outOfMemoryErrorHandler is set */
static void defaultOutOfMemoryErrorHandler(int errorCode, char *message, int exitStatus, char *file, int line) {
    log_fatal("Error code: %d, Message: '%s' in file %s", errorCode, message, file);
    exit(exitStatus);
}

/* Inject the function to call when fatalErrors occur */
static void (*outOfMemoryErrorHandler)(int errCode, char *mess, int exitStatus, char *file, int line) = defaultOutOfMemoryErrorHandler;
void setOutOfMemoryErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus, char *file, int line)) {
    outOfMemoryErrorHandler = function;
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


/* *****************************************************************

   Memory - this memory type has a dynamic area allocated separately from the Memory
            struct in which allocation takes place by moving the index.

 */

void memoryInit(Memory *memory, char *name, size_t size) {
    ENTER();
    log_debug("Init %s with new name '%s' and size %d", memory->name, name, size);
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
        outOfMemoryErrorHandler(ERR_NO_MEMORY, memory->name, EXIT_FAILURE, __FILE__, __LINE__);
    }
    memory->index += count*size;
    if (memory->index > memory->max)
        memory->max = memory->index;

    return pointer;
}

void *memoryAlloc(Memory *memory, size_t size) {
    return memoryAllocc(memory, 1, size);
}

bool memoryIsBetween(Memory *memory, void *pointer, int low, int high) {
    return pointer >= (void *)&memory->area[low] && pointer <= (void *)&memory->area[high];
}

static bool isInMemory(Memory *memory, void *pointer) {
    return memoryIsBetween(memory, pointer, 0, memory->index);
}

size_t memoryFreeUntil(Memory *memory, void *pointer) {
    assert(isInMemory(memory, pointer));
    /* Ensure we're not freeing beyond current allocations (marker must be <= current index) */
    int markerOffset = (char *)pointer - (char *)memory->area;
    if (markerOffset > memory->index) {
        log_fatal("Attempting to free '%s' arena until offset %d, but current index is only %d.",
                  memory->name, markerOffset, memory->index);
        log_fatal("This means the marker is beyond allocated memory - likely a marker from a different arena or corrupted.");
        assert(markerOffset <= memory->index);
    }
    int oldIndex = memory->index;
    memory->index = markerOffset;
    return oldIndex - memory->index;  // Amount freed (rolled back)
}

static bool memoryPointerIsFreed(Memory *memory, void *pointer) {
    return memoryIsBetween(memory, pointer, memory->index, memory->size);
}

/* Reallocates the most recently allocated area in 'memory' to be different size */
void *memoryRealloc(Memory *memory, void *pointer, size_t oldSize, size_t newSize) {
    /* Arena allocators can only resize the most recent allocation (top-of-stack).
     * If this fails, check if ppmFreeUntil() was called too late, freeing allocations
     * made AFTER the buffer being resized. The buffer must be at top-of-stack to grow. */
    if (pointer != &memory->area[memory->index-oldSize]) {
        log_fatal("Attempting to resize buffer %p (size=%zu) in '%s' arena, but it is not the most recent allocation.",
                  pointer, oldSize, memory->name);
        log_fatal("Expected buffer at %p (index=%d - oldSize=%zu = %d), but current top-of-stack is at %p (index=%d).",
                  &memory->area[memory->index-oldSize], memory->index, oldSize, memory->index - (int)oldSize,
                  &memory->area[memory->index], memory->index);
        log_fatal("This usually means allocations made after the buffer need to be freed first (e.g., move ppmFreeUntil() earlier).");
        assert(pointer == &memory->area[memory->index-oldSize]);
    }
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

int ppmFreeUntil(void *pointer) {
    return memoryFreeUntil(&ppmMemory, pointer);
}

bool ppmIsFreedPointer(void *pointer) {
    return memoryPointerIsFreed(&ppmMemory, pointer);
}


/* CX */
void initCxMemory(size_t size) {
    memoryInit(&cxMemory, "cxMemory", size);
}

void *cxAlloc(size_t size) {
    return memoryAllocc(&cxMemory, size, 1);
}

bool cxMemoryPointerIsBetween(void *pointer, int low, int high) {
    return memoryIsBetween(&cxMemory, pointer, low, high);
}

void cxFreeUntil(void *pointer) {
    (void)memoryFreeUntil(&cxMemory, pointer);
}

void printMemoryStatisticsFor(Memory *memory) {
    printf("Max memory use for %s : %d\n", memory->name, memory->max);
}

void printMemoryStatistics(void) {
    printMemoryStatisticsFor(&cxMemory);
    printMemoryStatisticsFor(&ppmMemory);
}
