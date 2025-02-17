#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>



/* ************************ Types ******************************** */

typedef struct memory {
    char   *name;              /* String representing the name of the memory */
    bool  (*overflowHandler)(int n); /* Should return true if more memory was possible to acquire */
    int     index;
    size_t  size;
    char   *area;
} Memory;


/* pre-processor macro definitions simple memory allocations in separate PPM-memory */
extern void *ppmAlloc(size_t size);
extern void *ppmAllocc(int count, size_t size);
extern void *ppmReallocc(void *pointer, int newCount, size_t size, int oldCount);
extern void  ppmFreeUntil(void *pointer);
extern bool ppmIsFreedPointer(void *pointer);



extern void memoryInit(Memory *memory, char *name, bool (*overflowHandler)(int n), int size);
extern void *memoryAllocc(Memory *memory, int count, size_t size);
extern void *memoryAlloc(Memory *memory, size_t size);

extern void *memoryRealloc(Memory *memory, void *pointer, size_t oldSize, size_t newSize);
extern bool memoryIsBetween(Memory *memory, void *pointer, int low, int high);
extern void memoryFreeUntil(Memory *memory, void *pointer);
extern void memoryResized(Memory *memory);

/***********************************************************************/

extern jmp_buf memoryResizeJumpTarget;


extern Memory ppmMemory;


/* Inject some error functions to remove linkage dependency */
extern void setFatalErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus,
                               char *file, int line));
extern void setInternalCheckFailHandlerForMemory(void (*function)(char *expr, char *file, int line));

/***********************************************************************/
/* New, so far unused, new Memory handling */

#define DONT_USE_NEW_CXMEMORY

typedef struct {
    const char *name;
    int size;
    int top;
    void **blocks;
    int pendingFlushIndex;
    size_t *sizes;
    size_t allocated;
    size_t flushed;
} FlushableMemory;

extern void initFlushableMemory(FlushableMemory *memory, const char *name);
extern void *allocateFlushableMemory(FlushableMemory *memory, size_t size);
extern void freeFlushableMemoryUntil(FlushableMemory *memory, void *pointer);
extern bool flushableMemoryIsFreed(FlushableMemory *memory, void *pointer);
extern bool memoryWouldBeFlushed(FlushableMemory *memory, void *pointer);
extern void markForFlushing(FlushableMemory *memory, void *pointer);
extern void flushPendingMemory(FlushableMemory *memory);

/* CX memory functions */
#ifdef USE_NEW_CXMEMORY
extern FlushableMemory cxMemory;

extern void markCxMemoryForFlushing(void *pointer);
extern void flushPendingCxMemory(void);
extern void printMemoryStatistics(FlushableMemory *memory);

#else
extern Memory cxMemory;

extern bool cxMemoryOverflowHandler(int n);
extern bool cxMemoryHasEnoughSpaceFor(size_t bytes);
extern void cxFreeUntil(void *until);

extern void printMemoryStatistics(Memory *memory);
#endif

extern void initCxMemory(size_t size);
extern void *cxAlloc(size_t size);
extern bool cxMemoryPointerIsBetween(void *pointer, int low, int high);
extern bool cxMemoryIsFreed(void *pointer);


#endif
