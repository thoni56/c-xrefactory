#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include "stdinc.h"
#include "constants.h"


/* ************************ Types ******************************** */

/* Two uses: cxMemory & options... */
typedef struct memory {
    char    *name;              /* String representing the name of the memory */
    bool	(*overflowHandler)(int n); /* Should return true if more memory was possible to acquire */
    int     index;
    int		size;
    double  block[SIZE_optMemory];	//  double in order to get it properly aligned
} Memory;

/* The "trail" seems to only be used for Java so ignore it for now... */
typedef struct freeTrail {
    void             (*action)(void*);
    void             *pointer;
    struct freeTrail *next;
} FreeTrail;

typedef struct codeBlock {
    int              firstFreeIndex;
    struct freeTrail *trail;
    struct codeBlock *outerBlock;
} CodeBlock;


/* ******************** a simple memory handler ************************ */


/**********************************************************************

    Stack memory synchronized with program block structure. New block
    with stackMemoryBlockStart();

    Convenience functions:
*/
#define StackMemoryAlloc(t) ((t*) stackMemoryAlloc(sizeof(t)))
#define StackMemoryAllocC(n, t) ((t*) stackMemoryAlloc((n)*sizeof(t)))



/* pre-processor macro definitions allocations */
extern void *ppmAlloc(size_t size);
extern void *ppmAllocc(int count, size_t size);
extern void *ppmRealloc(int count, size_t newSize, size_t oldSize);
extern void *ppmReallocc(void *pointer, int newCount, size_t size, int oldCount);
extern void  ppmFreeUntil(void *pointer);
extern bool ppmIsFreedPointer(void *pointer);

/* java class-file read allocations ( same memory as cpp !!!!!!!! ) */
#define CF_ALLOC(pointer, type)         {pointer = smAlloc(&ppmMemory, sizeof(type));}
#define CF_ALLOCC(pointer, count, type) {pointer = smAllocc(&ppmMemory, count, sizeof(type));}


/* ************************************************************************** */
/* New type of static memory */
typedef struct {
    char *name;
    size_t size;
    int index;
    char *area;
} Memory2;

extern void smInit(Memory2 *memory, char *name, size_t size);
extern void *smAllocc(Memory2 *memory, int count, size_t size);
extern void *smAlloc(Memory2 *memory, size_t size);
extern void *smRealloc(Memory2 *memory, void *pointer, size_t oldSize, size_t newSize);
extern void *smReallocc(Memory2 *memory, void *pointer, int newCount, size_t size, int oldCount);
extern void smFreeUntil(Memory2 *memory, void *pointer);
extern bool smIsFreedPointer(Memory2 *memory, void *pointer);


/***********************************************************************/

/* DM (Dynamic Memory) areas, can possibly expand using overflow handler */

extern CodeBlock *currentBlock;

extern jmp_buf memoryResizeJumpTarget;

extern Memory *cxMemory;

extern int olcxMemoryAllocatedBytes;


/* SM (Static memory) areas */
extern char stackMemory[];

extern Memory2 ppmMemory;


/* Inject some error functions to remove linkage dependency */
extern void setFatalErrorHandlerForMemory(void (*function)(int errCode, char *mess, int exitStatus,
                               char *file, int line));
extern void setInternalCheckFailHandlerForMemory(void (*function)(char *expr, char *file, int line));
extern void setErrorHandlerForMemory(void (*function)(int code, char *message));


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


/* on-line dialogs allocation */
extern void olcx_memory_init();
extern void *olcx_memory_soft_allocc(int count, size_t size);
extern void *olcx_memory_allocc(int count, size_t size);
extern void *olcx_alloc(size_t size);
extern void olcx_memory_free(void *pointer, size_t size);


/* editor allocations, for now, store it in olcxmemory */
extern void *editorAlloc(size_t size);
extern void  editorFree(void *pointer, size_t size);


extern void initMemory(Memory *memory, char *name, bool (*overflowHandler)(int n), int size);
extern void memoryResized(void);
extern bool cxMemoryOverflowHandler(int n);

extern void addToTrail(void (*action)(void*), void *p, bool needTrailOnTopLevel);
extern void removeFromTrailUntil(FreeTrail *untilP);

extern void initOuterCodeBlock(void);
extern void *stackMemoryAlloc(int size);
extern char *stackMemoryPushString(char *s);

extern void beginBlock(void);
extern void endBlock(void);
extern int nestingLevel(void);

extern bool isMemoryFromPreviousBlock(void *ppp);
extern bool isFreedPointer(void *ptr);

#endif
