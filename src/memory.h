#ifndef _MEMMAC__H
#define _MEMMAC__H

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
#define StackMemoryFree(p)              { }


/**********************************************************************
  SM = Static Memory - once allocated cannot expand

*/

#define SM_INIT(mem) {mem##Index = 0;}
#define SM_ALLOCC(mem,p,n,t) {\
    assert( (n) >= 0);\
    /* memset(mem+mem##Index,0,(n)*sizeof(t)); */\
    mem##Index = ((char*)ALIGNMENT(mem+mem##Index,STANDARD_ALIGNMENT)) - mem;\
    if (mem##Index+(n)*sizeof(t) >= SIZE_##mem) {\
        fatalError(ERR_NO_MEMORY,#mem, XREF_EXIT_ERR);\
    }\
    p = (t*) (mem + mem##Index);\
    /* memset(p,0,(n)*sizeof(t)); / * for detecting any bug */\
    mem##Index += (n)*sizeof(t);\
}
#define SM_ALLOC(mem,p,t) {SM_ALLOCC(mem,p,1,t);}
#define SM_REALLOCC(mem,p,n,t,oldn) {\
    assert(((char *)(p)) + (oldn)*sizeof(t) == mem + mem##Index);\
    mem##Index = ((char*)p) - mem;\
    SM_ALLOCC(mem,p,n,t);\
}
#define SM_FREE_UNTIL(mem,p) {\
    assert((p)>=mem && (p)<= mem+mem##Index);\
    mem##Index = ((char*)(p))-mem;\
}
#define SM_FREED_POINTER(mem,ppp) (\
    ((char*)ppp) >= mem + mem##Index && ((char*)ppp) < mem + SIZE_##mem \
)


/**********************************************************************
   DM = Dynamic Memory - can expand using overflow handler

*/

#define DM_FREE_SPACE(mem,n) (mem->index+(n) < mem->size)

#define DM_IS_BETWEEN(mem,ppp,iii,jjj) (\
    ((char*)ppp) >= ((char*)&mem->b) + (iii) && ((char*)ppp) < ((char*)&mem->b) + (jjj) \
)
#define DM_FREED_POINTER(mem,ppp) DM_IS_BETWEEN(mem,ppp,mem->index,mem->size)

#define DM_INIT(mem) {mem->index = 0;}
#define DM_ALLOCC(mem,p,n,type) {\
    assert( (n) >= 0);\
    mem->index = ((char*)ALIGNMENT(((char*)&mem->b)+mem->index,STANDARD_ALIGNMENT)) - ((char*)&mem->b);\
    if (mem->index+(n)*sizeof(type) >= mem->size) {\
        if (mem->overflowHandler(n)) memoryResize();        \
        else fatalError(ERR_NO_MEMORY,#mem, XREF_EXIT_ERR);\
    }\
    p = (type*) (((char*)&mem->b) + mem->index);\
    mem->index += (n)*sizeof(type);\
}
#define DM_ALLOC(mem,p,t) {DM_ALLOCC(mem,p,1,t);}
#define DM_REALLOCC(mem,p,n,t,oldn) {\
    assert(((char *)(p)) + (oldn)*sizeof(t) == &mem->b + mem->i);\
    mem->i = ((char*)p) - &mem->b;\
    DM_ALLOCC(mem,p,n,t);\
}
#define DM_FREE_UNTIL(mem,p) {\
    assert((p)>= ((char*)&mem->b) && (p)<= ((char*)&mem->b)+mem->index);\
    mem->index = ((char*)(p)) - ((char*)&mem->b);\
}

/* editor allocations, for now, store it in olcxmemory */
#define ED_ALLOCC(p,n,t) OLCX_ALLOCC(p,n,t)
#define ED_ALLOC(p,t) ED_ALLOCC(p,1,t)
#define ED_FREE(p,size) OLCX_FREE(p,size)


/* ************* a supplementary level with free-lists ******************** */

/* This is only used for olcxMemory so "mem" is always olcxMemory... */
#define REAL_MEMORY_INIT(mem) {mem##AllocatedBytes = 0; CHECK_INIT();}

#define REAL_MEMORY_SOFT_ALLOCC(mem,p,nn,t) {\
    int n = (nn) * sizeof(t);\
    if (n+mem##AllocatedBytes > SIZE_##mem) {\
        p = NULL;\
    } else {\
        p = (t*) malloc(n);\
        mem##AllocatedBytes += n;\
        CHECK_ALLOC(p, n);\
    }\
}
#define REAL_MEMORY_FREE(mem,p,nn) {\
    CHECK_FREE(p, nn);\
    mem##AllocatedBytes -= nn; /* decrement first, free after (because of nn=strlen(p)) */\
    free(p);\
}



/* ********************************************************************** */

/* Here are some macros that adds consistency checks to olcx-memory handling */

#ifdef OLCX_MEMORY_CHECK
#define CHECK_INIT() {s_olcx_check_arrayi=0;}
#define CHECK_ALLOC(p, n) {\
    if (s_olcx_check_arrayi == -1) assert(0);\
    s_olcx_check_array[s_olcx_check_arrayi] = p;\
    s_olcx_check_array_sizes[s_olcx_check_arrayi] = n;\
    s_olcx_check_arrayi ++;\
    assert(s_olcx_check_arrayi<OLCX_CHECK_ARRAY_SIZE-2);\
}
#define CHECK_FREE(p, nn) {\
    int _itmpi, _nnlen;\
    _nnlen = nn;\
    if (nn == 0) error(ERR_INTERNAL, "Freeing chunk of size 0");\
    for (_itmpi=0; _itmpi<s_olcx_check_arrayi; _itmpi++) {\
        if (s_olcx_check_array[_itmpi] == p) {\
            s_olcx_check_array[_itmpi]=NULL;\
            if (_nnlen != s_olcx_check_array_sizes[_itmpi]) {\
                sprintf(tmpBuff, "Cell %d allocated with size %d and freed with %d\n", _itmpi, s_olcx_check_array_sizes[_itmpi], _nnlen);\
                fatalError(ERR_INTERNAL, tmpBuff, XREF_EXIT_ERR);\
            }\
            break;\
        }\
    }\
    if (_itmpi==s_olcx_check_arrayi) fatalError(ERR_INTERNAL, "Freeing unallocated cell", XREF_EXIT_ERR);\
}
#define CHECK_FINAL() {\
    int _itmpi;\
    for (_itmpi=0; _itmpi<s_olcx_check_arrayi; _itmpi++) {\
        if (s_olcx_check_array[_itmpi]!=NULL) {\
            sprintf(tmpBuff, "unfreed cell #%d\n", _itmpi);\
            error(ERR_INTERNAL, tmpBuff;\
        }\
    }\
}
#else
#define CHECK_INIT() {}
#define CHECK_ALLOC(p, n) {}
#define CHECK_FREE(p, n) {}
#define CHECK_FINAL() {}
#endif


/* ********************************************************************** */

typedef struct freeTrail {
    void             (*action)(void*);
    void             *p;
    struct freeTrail *next;
} S_freeTrail;

typedef struct memory {
    bool	(*overflowHandler)(int n);
    int     index;
    int		size;
    double  b;		//  double in order to get it properly aligned
} Memory;

typedef struct topBlock {
    int              firstFreeIndex;
    int              tmpMemoryBasei;
    struct freeTrail *trail;
    struct topBlock  *previousTopBlock;
} S_topBlock;


extern Memory *cxMemory;
extern S_topBlock *s_topBlock;

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
extern void removeFromTrailUntil(S_freeTrail *untilP);

extern void stackMemoryInit(void);
extern void *stackMemoryAlloc(int size);
extern void *stackMemoryPush(void *p, int size);
extern char *stackMemoryPushString(char *s);
extern void stackMemoryBlockStart(void);
extern void stackMemoryBlockFree(void);
extern int nestingLevel(void);

extern bool memoryFromPreviousBlock(void *ppp);
extern bool freedPointer(void *ptr);

#endif
