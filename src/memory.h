#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include "stdinc.h"


/* ******************** a simple memory handler ************************ */

#define ALIGNMENT_OFF(xxx,align) (align-1-((((uintptr_t)(xxx))-1) & (align-1)))
#define ALIGNMENT(xxx,align) (((char*)(xxx))+ALIGNMENT_OFF(xxx,align))


/**********************************************************************

    Stack memory synchronized with program block structure. New block
    with stackMemoryBlockStart();

*/
#define StackMemoryAlloc(t) ((t*) stackMemoryAlloc(sizeof(t)))
#define StackMemoryAllocC(n, t) ((t*) stackMemoryAlloc((n)*sizeof(t)))


/**********************************************************************
  SM = Static Memory - once allocated cannot expand

*/

#define SM_INIT(mem) {mem##Index = 0;}
#define SM_ALLOCC(mem,p,n,t) {                                          \
        assert( (n) >= 0);                                              \
        /* memset(mem+mem##Index,0,(n)*sizeof(t)); */                   \
        mem##Index = ((char*)ALIGNMENT(mem+mem##Index,STANDARD_ALIGNMENT)) - mem; \
        if (mem##Index+(n)*sizeof(t) >= SIZE_##mem) {                   \
            fatalError(ERR_NO_MEMORY,#mem, XREF_EXIT_ERR);              \
        }                                                               \
        p = (t*) (mem + mem##Index);                                    \
        /* memset(p,0,(n)*sizeof(t)); / * for detecting any bug */      \
        mem##Index += (n)*sizeof(t);                                    \
    }
#define SM_ALLOC(mem,p,t) {SM_ALLOCC(mem,p,1,t);}
#define SM_REALLOCC(mem,p,n,t,oldn) {                                   \
        assert(((char *)(p)) + (oldn)*sizeof(t) == mem + mem##Index);   \
        mem##Index = ((char*)p) - mem;                                  \
        SM_ALLOCC(mem,p,n,t);                                           \
}
#define SM_FREE_UNTIL(mem,p) {                      \
        assert((p)>=mem && (p)<= mem+mem##Index);   \
        mem##Index = ((char*)(p))-mem;              \
    }
#define SM_FREED_POINTER(mem,ppp) (                                     \
        ((char*)ppp) >= mem + mem##Index && ((char*)ppp) < mem + SIZE_##mem \
    )


/**********************************************************************
   DM = Dynamic Memory - can expand using overflow handler

*/

#define DM_ENOUGH_SPACE_FOR(memory, bytes) (memory->index+(bytes) < memory->size)

#define DM_IS_BETWEEN(memory, pointer, low, high) (                     \
        ((char*)pointer) >= ((char*)&memory->block) + (low) && ((char*)pointer) < ((char*)&memory->block) + (high) \
    )
#define DM_FREED_POINTER(memory, pointer) DM_IS_BETWEEN(memory, pointer, memory->index, memory->size)

#define DM_INIT(memory) {memory->index = 0;}
#define DM_ALLOCC(memory, variable, count, type) {                      \
        assert((count) >= 0);                                           \
        memory->index = ((char*)ALIGNMENT(((char*)&memory->block)+memory->index,STANDARD_ALIGNMENT)) - ((char*)&memory->block); \
        if (memory->index+(count)*sizeof(type) >= memory->size) {       \
            if (memory->overflowHandler(count)) memoryResize();         \
            else fatalError(ERR_NO_MEMORY,#memory, XREF_EXIT_ERR);      \
        }                                                               \
        variable = (type*) (((char*)&memory->block) + memory->index);   \
        memory->index += (count)*sizeof(type);                          \
    }
#define DM_ALLOC(memory, variable, type) {DM_ALLOCC(memory,variable,1,type);}
#define DM_FREE_UNTIL(memory, pointer) {                                \
        assert((pointer)>= ((char*)&memory->block) && (pointer)<= ((char*)&memory->block)+memory->index); \
        memory->index = ((char*)(pointer)) - ((char*)&memory->block);   \
    }

/* editor allocations, for now, store it in olcxmemory */
#define ED_ALLOCC(p,n,t) OLCX_ALLOCC(p,n,t)
#define ED_ALLOC(p,t) ED_ALLOCC(p,1,t)
#define ED_FREE(p,size) OLCX_FREE(p,size)


/* ************* a supplementary level with free-lists ******************** */

/* This is only used for olcxMemory so "mem" is always olcxMemory... */
#define REAL_MEMORY_INIT(mem) {mem##AllocatedBytes = 0;}

#define REAL_MEMORY_SOFT_ALLOCC(memory, variable, count, type) {    \
        int n = (count) * sizeof(type);                             \
        if (n+memory##AllocatedBytes > SIZE_##memory) {             \
            variable = NULL;                                        \
        } else {                                                    \
            variable = (type*) malloc(n);                           \
            memory##AllocatedBytes += n;                            \
        }                                                           \
    }
#define REAL_MEMORY_FREE(mem,p,nn) {                                    \
        /* decrement first, free after (because of nn=strlen(p)) */     \
        mem##AllocatedBytes -= nn;                                      \
        free(p);                                                        \
    }


/* ********************************************************************** */

/* pre-processor macro definitions allocations */
#define PP_ALLOC(p,t)           {SM_ALLOC(ppmMemory,p,t);}
#define PP_ALLOCC(p,n,t)        {SM_ALLOCC(ppmMemory,p,n,t);}
#define PP_REALLOCC(p,n,t,on)	{SM_REALLOCC(ppmMemory,p,n,t,on);}
#define PP_FREE_UNTIL(p)        {SM_FREE_UNTIL(ppmMemory,p);}

/* java class-file read allocations ( same memory as cpp !!!!!!!! ) */
#define CF_ALLOC(p,t)           {SM_ALLOC(ppmMemory,p,t);}
#define CF_ALLOCC(p,n,t)        {SM_ALLOCC(ppmMemory,p,n,t);}

/* file table allocations */
#define FT_ALLOC(p,t)           {SM_ALLOC(ftMemory,p,t);}
#define FT_ALLOCC(p,n,t)        {SM_ALLOCC(ftMemory,p,n,t);}

/* cross - references global symbols allocations */
#define CX_ALLOC(p,t)           {DM_ALLOC(cxMemory,p,t);}
#define CX_ALLOCC(p,n,t)        {DM_ALLOCC(cxMemory,p,n,t);}
#define CX_FREE_UNTIL(p)        {DM_FREE_UNTIL(cxMemory,p);}

/* options allocations */
#define OPT_ALLOC(p,t)          {DM_ALLOC(((Memory*)&options.pendingMemory),p,t);}
#define OPT_ALLOCC(p,n,t)       {DM_ALLOCC(((Memory*)&options.pendingMemory),p,n,t);}

/* on-line dialogs allocation */
#define OLCX_ALLOCC(p,n,t) {                                \
        REAL_MEMORY_SOFT_ALLOCC(olcxMemory, p, n, t);       \
        while (p==NULL) {                                   \
            freeOldestOlcx();                               \
            REAL_MEMORY_SOFT_ALLOCC(olcxMemory, p, n, t);   \
        }                                                   \
    }
#define OLCX_ALLOC(p,t) OLCX_ALLOCC(p,1,t)
#define OLCX_FREE(p,size) REAL_MEMORY_FREE(olcxMemory, p, size)


/* ********************************************************************** */

typedef struct freeTrail {
    void             (*action)(void*);
    void             *pointer;
    struct freeTrail *next;
} FreeTrail;

typedef struct memory {
    bool	(*overflowHandler)(int n); /* Should return true if more memory was possible to acquire */
    int     index;
    int		size;
    double  block;		//  double in order to get it properly aligned
} Memory;

typedef struct topBlock {
    int              firstFreeIndex;
    int              tmpMemoryBasei;
    struct freeTrail *trail;
    struct topBlock  *previousTopBlock;
} TopBlock;


extern Memory *cxMemory;
extern TopBlock *s_topBlock;

extern jmp_buf memoryResizeJumpTarget;

extern char tmpWorkMemory[];
extern int tmpWorkMemoryIndex;
extern char ftMemory[];
extern int ftMemoryIndex;
extern char tmpMemory[];


/* Inject some error functions to remove linkage dependency */
extern void memoryUseFunctionForFatalError(void (*function)(int errCode, char *mess, int exitStatus));
extern void memoryUseFunctionForInternalCheckFail(void (*function)(char *expr, char *file, int line));
extern void memoryUseFunctionForError(void (*function)(int code, char *message));

extern void initMemory(Memory *memory, bool (*overflowHandler)(int n), int size);
extern void memoryResize(void);
extern bool cxMemoryOverflowHandler(int n);

extern void addToTrail (void (*action)(void*),  void *p);
extern void removeFromTrailUntil(FreeTrail *untilP);

extern void stackMemoryInit(void);
extern void *stackMemoryAlloc(int size);
extern char *stackMemoryPushString(char *s);
extern void stackMemoryBlockStart(void);
extern void stackMemoryBlockEnd(void);
extern int nestingLevel(void);

extern bool memoryFromPreviousBlock(void *ppp);
extern bool freedPointer(void *ptr);

#endif
