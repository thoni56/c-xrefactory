#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include "stdinc.h"


/* ************************ Types ******************************** */

typedef struct memory {
    char    *name;              /* String representing the name of the memory */
    bool	(*overflowHandler)(int n); /* Should return true if more memory was possible to acquire */
    int     index;
    int		size;
    double  block;		//  double in order to get it properly aligned
    /* WTF: TODO It seems like the actual area is in the array *after*
     * this struct, so it overruns into the memory after, this won't
     * work in all compilers (re-arranging declarations) or
     * architectures... */
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

#define ALIGNMENT_PADDING(xxx,align) (align-1-((((uintptr_t)(xxx))-1) & (align-1)))
#define ALIGNMENT(xxx,align) (((char*)(xxx))+ALIGNMENT_PADDING(xxx,align))


/**********************************************************************

    Stack memory synchronized with program block structure. New block
    with stackMemoryBlockStart();

*/
#define StackMemoryAlloc(t) ((t*) stackMemoryAlloc(sizeof(t)))
#define StackMemoryAllocC(n, t) ((t*) stackMemoryAlloc((n)*sizeof(t)))


/**********************************************************************
  SM = Static Memory? - once allocated cannot expand?

  Have a separate int variable called mem##Index instead of the Memory
  struct used by DM (merge?)

*/

#define SM_INIT(memory) {memory##Index = 0;}
#define SM_ALLOCC(memory, pointer, count, type) {                       \
        assert( (count) >= 0);                                          \
        /* memset(mem+mem##Index,0,(n)*sizeof(t)); */                   \
        memory##Index = ((char*)ALIGNMENT(memory+memory##Index,STANDARD_ALIGNMENT)) - memory; \
        if (memory##Index+(count)*sizeof(type) >= SIZE_##memory) {      \
            fatalError(ERR_NO_MEMORY,#memory, XREF_EXIT_ERR);           \
        }                                                               \
        pointer = (type*) (memory + memory##Index);                     \
        /* memset(p,0,(n)*sizeof(t)); / * for detecting any bug */      \
        memory##Index += (count)*sizeof(type);                          \
    }
#define SM_ALLOC(memory, pointer, type) {SM_ALLOCC(memory,pointer,1,type);}
#define SM_REALLOCC(memory, pointer, count, type, oldCount) {            \
        assert(((char *)(pointer)) + (oldCount)*sizeof(type) == memory + memory##Index); \
        memory##Index = ((char*)pointer) - memory;                      \
        SM_ALLOCC(memory,pointer,count,type);                           \
    }
#define SM_FREE_UNTIL(memory, pointer) {                                \
        assert((pointer)>=memory && (pointer)<= memory+memory##Index);  \
        memory##Index = ((char*)(pointer))-memory;                      \
    }


/* pre-processor macro definitions allocations */
#define PPM_ALLOC(pointer, type)         {SM_ALLOC(ppmMemory, pointer, type);}
#define PPM_ALLOCC(pointer, count, type) {SM_ALLOCC(ppmMemory, pointer, count, type);}
#define PPM_REALLOCC(pointer, count, type, oldCount)	{SM_REALLOCC(ppmMemory, pointer, count, type, oldCount);}
#define PPM_FREE_UNTIL(pointer)          {SM_FREE_UNTIL(ppmMemory, pointer);}
#define PPM_FREED_POINTER(pointer) (                                     \
        ((char*)pointer) >= ppmMemory + ppmMemoryIndex && ((char*)pointer) < ppmMemory + SIZE_ppmMemory \
    )

/* java class-file read allocations ( same memory as cpp !!!!!!!! ) */
#define CF_ALLOC(pointer, type)         {SM_ALLOC(ppmMemory, pointer, type);}
#define CF_ALLOCC(pointer, count, type) {SM_ALLOCC(ppmMemory, pointer, count, type);}

/* file table allocations */
#define FT_ALLOC(pointer, type)         {SM_ALLOC(ftMemory, pointer, type);}
#define FT_ALLOCC(pointer, count, type) {SM_ALLOCC(ftMemory, pointer, count, type);}

/* macro bodies */
#define MB_INIT()                       {SM_INIT(mbMemory);}
#define MB_ALLOCC(pointer, count, type) {SM_ALLOCC(mbMemory, pointer, count, type);}
#define MB_REALLOCC(pointer, count, type, oldCount)	{SM_REALLOCC(mbMemory, pointer, count, type, oldCount);}
#define MB_FREE_UNTIL(pointer)          {SM_FREE_UNTIL(mbMemory, pointer);}


/**********************************************************************
   DM = Dynamic Memory - can expand using overflow handler
*/

#define DM_ENOUGH_SPACE_FOR(memory, bytes) (memory->index+(bytes) < memory->size)

#define DM_IS_BETWEEN(memory, pointer, low, high) (                     \
        ((char*)pointer) >= ((char*)&memory->block) + (low) && ((char*)pointer) < ((char*)&memory->block) + (high) \
    )
#define DM_FREED_POINTER(memory, pointer) DM_IS_BETWEEN(memory, pointer, memory->index, memory->size)

#define DM_ALLOCC(memory, variable, count, type) {                      \
        assert((count) >= 0);                                           \
        memory->index = ((char*)ALIGNMENT(((char*)&memory->block)+memory->index,STANDARD_ALIGNMENT)) - ((char*)&memory->block); \
        if (memory->index+(count)*sizeof(type) >= memory->size) {       \
            if (memory->overflowHandler(count)) memoryResized();        \
            else fatalError(ERR_NO_MEMORY,#memory, XREF_EXIT_ERR);      \
        }                                                               \
        variable = (type*) (((char*)&memory->block) + memory->index);   \
        memory->index += (count)*sizeof(type);                          \
    }
#define DM_ALLOC(memory, variable, type) variable = dm_alloc(memory, sizeof(type));
#define DM_FREE_UNTIL(memory, pointer) {                                \
        assert((pointer)>= ((char*)&memory->block) && (pointer)<= ((char*)&memory->block)+memory->index); \
        memory->index = ((char*)(pointer)) - ((char*)&memory->block);   \
    }


/* cross-references global symbols allocations */
#define CX_ALLOC(pointer, type)         pointer = dm_alloc(cxMemory, sizeof(type));
#define CX_ALLOCC(pointer, count, type) pointer = dm_allocc(cxMemory, count, sizeof(type));
#define CX_FREE_UNTIL(pointer)          {DM_FREE_UNTIL(cxMemory, pointer);}
#define CX_FREED_POINTER(pointer)       DM_FREED_POINTER(cxMemory, pointer)

/* options allocations */
#define OPT_ALLOC(pointer, type)         pointer = dm_alloc(&options.memory, sizeof(type));
#define OPT_ALLOCC(pointer, count, type) pointer = dm_allocc(&options.memory, count, sizeof(type));

/* editor allocations, for now, store it in olcxmemory */
#define ED_ALLOCC(pointer, count, type) { pointer = olcx_memory_allocc(count, sizeof(type)); }
#define ED_ALLOC(pointer, type) ED_ALLOCC(pointer, 1, type)
#define ED_FREE(pointer, size) olcx_memory_free(pointer, size)


/***********************************************************************/

/* DM (Dynamic Memory) areas */

extern CodeBlock *currentBlock;

extern jmp_buf memoryResizeJumpTarget;

extern Memory *cxMemory;

extern int olcxMemoryAllocatedBytes;


/* SM (Static memory) areas */
extern char workMemory[];

extern char ftMemory[];
extern int ftMemoryIndex;

extern char ppmMemory[];
extern int ppmMemoryIndex;

extern char mbMemory[];
extern int mbMemoryIndex;

extern char tmpMemory[];


/* Inject some error functions to remove linkage dependency */
extern void memoryUseFunctionForFatalError(void (*function)(int errCode, char *mess, int exitStatus));
extern void memoryUseFunctionForInternalCheckFail(void (*function)(char *expr, char *file, int line));
extern void memoryUseFunctionForError(void (*function)(int code, char *message));

extern void dm_init(Memory *memory, char *name);
extern void *dm_alloc(Memory *memory, size_t size);
extern void *dm_allocc(Memory *memory, int count, size_t size);

/* on-line dialogs allocation */
extern void olcx_memory_init();
extern void *olcx_memory_soft_allocc(int count, size_t size);
extern void *olcx_memory_allocc(int count, size_t size);
extern void *olcx_alloc(size_t size);
extern void olcx_memory_free(void *pointer, size_t size);

extern void initMemory(Memory *memory, bool (*overflowHandler)(int n), int size);
extern void memoryResized(void);
extern bool cxMemoryOverflowHandler(int n);

extern void addToTrail(void (*action)(void*), void *p);
extern void removeFromTrailUntil(FreeTrail *untilP);

extern void stackMemoryInit(void);
extern void *stackMemoryAlloc(int size);
extern char *stackMemoryPushString(char *s);

extern void beginBlock(void);
extern void endBlock(void);
extern int nestingLevel(void);

extern bool isMemoryFromPreviousBlock(void *ppp);
extern bool freedPointer(void *ptr);

#endif
