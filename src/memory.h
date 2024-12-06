#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>



/* ************************ Types ******************************** */

/* Only use: cxMemory - possibility to just nuke down to an address */
typedef struct memory {
    char   *name;              /* String representing the name of the memory */
    bool  (*overflowHandler)(int n); /* Should return true if more memory was possible to acquire */
    int     index;
    size_t  size;
    char   *area;
} Memory;


/* ******************** a simple memory handler ************************ */


/* pre-processor macro definitions simple memory allocations in separate PPM-memory */
extern void *ppmAlloc(size_t size);
extern void *ppmAllocc(int count, size_t size);
extern void *ppmReallocc(void *pointer, int newCount, size_t size, int oldCount);
extern void  ppmFreeUntil(void *pointer);
extern bool ppmIsFreedPointer(void *pointer);


extern void smInit(Memory *memory, char *name, size_t size);
extern void *smAllocc(Memory *memory, int count, size_t size);
extern void *smAlloc(Memory *memory, size_t size);
extern void *smRealloc(Memory *memory, void *pointer, size_t oldSize, size_t newSize);
extern void *smReallocc(Memory *memory, void *pointer, int newCount, size_t size, int oldCount);
extern bool smIsBetween(Memory *memory, void *pointer, int low, int high);
extern void smFreeUntil(Memory *memory, void *pointer);
extern bool smIsFreedPointer(Memory *memory, void *pointer);


/***********************************************************************/

/* DM (Dynamic Memory) areas, can possibly expand using overflow handler */

extern jmp_buf memoryResizeJumpTarget;

extern Memory *cxMemory;

extern Memory ppmMemory;


/* Inject some error functions to remove linkage dependency */
extern void setFatalErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus,
                               char *file, int line));
extern void setInternalCheckFailHandlerForMemory(void (*function)(char *expr, char *file, int line));


/* DM - Dynamic Memory - multiple uses, see below */
extern void dm_init(Memory *memory, char *name);
extern void *dm_alloc(Memory *memory, size_t size);
extern void *dm_allocc(Memory *memory, int count, size_t size);
extern bool dm_enoughSpaceFor(Memory *memory, size_t bytes);
extern bool dm_isBetween(Memory *memory, void *pointer, int low, int high);
extern bool dm_isFreedPointer(Memory *memory, void *pointer);
extern void dm_freeUntil(Memory *memory, void *pointer);


/* cross-references global symbols allocations */
extern void *cxAlloc(size_t size);
extern void cxFreeUntil(void *until);
extern bool isFreedCxMemory(void *pointer);


extern void initMemory(Memory *memory, char *name, bool (*overflowHandler)(int n), int size);
extern void memoryResized(void);
extern bool cxMemoryOverflowHandler(int n);

#endif
