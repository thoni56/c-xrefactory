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
void cxMemoryResized(void) {
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
            cxMemoryResized();
        else
            fatalMemoryError(ERR_NO_MEMORY, memory->name, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    memory->index += count*size;
    return pointer;
}

void *memoryAlloc(Memory *memory, size_t size) {
    return memoryAllocc(memory, 1, size);
}

static bool pointerIsBetween(Memory *memory, void *pointer, size_t low, size_t high) {
    return pointer >= (void *)&memory->area[low] && pointer <= (void *)&memory->area[high];
}

static bool isInMemory(Memory *memory, void *pointer) {
    return pointerIsBetween(memory, pointer, 0, memory->index);
}

void memoryFreeUntil(Memory *memory, void *pointer) {
    assert(isInMemory(memory, pointer));
    memory->index = (char *)pointer - (char *)memory->area;
}

bool memoryPointerIsFreed(Memory *memory, void *pointer) {
    return pointerIsBetween(memory, pointer, memory->index, memory->size);
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
    // WTF: watchout, allocating newsize extra space for the implicitly contiguous area
    cxMemory = malloc(newsize + sizeof(Memory));
    if (cxMemory!=NULL) {
        memoryInit(cxMemory, "cxMemory", cxMemoryOverflowHandler, newsize);
    }
    log_debug("Reallocating cxMemory: %d -> %d", oldsize, newsize);

    return cxMemory != NULL;
}

/* *****************************************************************

   DM - direct memory allocates a Memory including the area in one go,
        see line "cxMemory = malloc(NEWSIZE + sizeof(Memory))" in
        cxMemoryOverflowhandler()

        This was not expected!!!

        We need to transform this to the form where Memory is a variable
        and the area is pointed to, not implicitly contiguous.

 */

/* CX */
void *cxAlloc(size_t size) {
    int previous_index;

    if (cxMemory->index+size >= cxMemory->size) {
        if (cxMemory->overflowHandler != NULL && cxMemory->overflowHandler(size))
            cxMemoryResized();
        else
            fatalMemoryError(ERR_NO_MEMORY, cxMemory->name, XREF_EXIT_ERR, __FILE__, __LINE__);
    }
    previous_index = cxMemory->index;
    cxMemory->index += size;
    // WTF: returns pointer in area and beyond, NOT in the allocated area it points to...
    return (void *) (((char*)&cxMemory->area) + previous_index);
}

bool cxMemoryHasEnoughSpaceFor(size_t bytes) {
    return cxMemory->index + bytes < cxMemory->size;
}

bool cxMemoryPointerIsBetween(void *pointer, int low, int high) {
    return pointer >= (void *)&cxMemory->area + low && pointer < (void *)&cxMemory->area + high;
}

bool isFreedCxMemory(void *pointer) {
    return cxMemoryPointerIsBetween(pointer, cxMemory->index, cxMemory->size);
}

void cxFreeUntil(void *pointer) {
    assert(pointer >= (void *)&cxMemory->area && pointer <= (void *)&cxMemory->area+cxMemory->index);
    cxMemory->index = (void *)pointer - (void *)&cxMemory->area;
}


bool smIsBetween(Memory *memory, void *pointer, int low, int high) {
    return pointer >= (void *)&memory->area[low] && pointer < (void *)&memory->area[high];
}

static bool smIsFreedPointer(Memory *memory, void *pointer) {
    return smIsBetween(memory, pointer, memory->index, memory->size);
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
    return smIsFreedPointer(&ppmMemory, pointer);
}
