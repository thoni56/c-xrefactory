#include "globals.h"

#include "memory.h"
#include "strFill.h"
#include "log.h"
#include "commons.h"
#include "misc.h"               /* for removeFromTrailUntil() */


/* With this as a separate function it is possible to catch memory resize longjmps */
void memoryResize(void) {
    longjmp(s_memoryResize,1);
}

/* ************************** Overflow Handlers ************************* */

int optionsOverflowHandler(int n) {
    fatalError(ERR_NO_MEMORY, "opiMemory", XREF_EXIT_ERR);
    return(1);
}

int cxMemoryOverflowHandler(int n) {
    int ofaktor,faktor,oldsize, newsize;
    S_memory *oldcxMemory;

    if (cxMemory!=NULL) {
        oldsize = cxMemory->size;
    } else {
        oldsize = 0;
    }

    ofaktor = oldsize / CX_MEMORY_CHUNK_SIZE;
    faktor = ((n>1)?(n-1):0)/CX_MEMORY_CHUNK_SIZE + 1; // 1 no patience to wait ;
    //& if (s_opt.cxMemoryFaktor>=1) faktor *= s_opt.cxMemoryFaktor;
    faktor += ofaktor;
    if (ofaktor*2 > faktor) faktor = ofaktor*2;
    newsize = faktor * CX_MEMORY_CHUNK_SIZE;
    oldcxMemory = cxMemory;
    if (oldcxMemory!=NULL) free(oldcxMemory);
    cxMemory = malloc(newsize + sizeof(S_memory));
    if (cxMemory!=NULL) {
        FILL_memory(cxMemory, cxMemoryOverflowHandler, 0, newsize, 0);
    }
    log_debug("Reallocating cxMemory: %d -> %d", oldsize, newsize);

    return(cxMemory!=NULL);
}

/* ***************************************************************** */

static void fillTopBlock(S_topBlock *topBlock, int firstFreeIndex, int tmpMemoryBasei, S_freeTrail *trail, S_topBlock *previousTopBlock) {
    topBlock->firstFreeIndex = firstFreeIndex;
    topBlock->tmpMemoryBasei = tmpMemoryBasei;
    topBlock->trail = trail;
    topBlock->previousTopBlock = previousTopBlock;
}

void stackMemoryInit(void) {
    s_topBlock = (S_topBlock *) memory;
    fillTopBlock(s_topBlock, sizeof(S_topBlock), 0, NULL, NULL);
}

void *stackMemoryAlloc(int size) {
    register int i;
    i = s_topBlock->firstFreeIndex;
    i = ((char *)ALIGNMENT(memory+i,STANDARD_ALIGNMENT))-memory;
    if (i+size < SIZE_workMemory) {
        s_topBlock->firstFreeIndex = i+size;
        return( & memory[i] );
    } else {
        fatalError(ERR_ST,"i+size > SIZE_workMemory,\n\tworking memory overflowed,\n\tread TROUBLES section of README file\n", XREF_EXIT_ERR);
        assert(0);
        return(NULL);
    }
}

void *stackMemoryPush(void *p, int size) {
    void *m;
    m = stackMemoryAlloc(size);
    memcpy(m,p,size);
    return(m);
}

void stackMemoryPop(void *p, int size) {
    int i;
    i = s_topBlock->firstFreeIndex;
    if (i-size < 0) {
        fprintf(stderr,"i-size < 0\n"); assert(0);
    }
    memcpy(p, & memory[i-size], size);
    s_topBlock->firstFreeIndex = i-size;
}

int *stackMemoryPushInt(int x) {
    /*fprintf(dumpOut,"pushing int %d\n", x);*/
    return((int*)stackMemoryPush(&x, sizeof(int)));
}

char *stackMemoryPushString(char *s) {
    /*fprintf(dumpOut,"pushing string %s\n",s);*/
    return((char*)stackMemoryPush(s, strlen(s)+1));
}

void stackMemoryBlockStart(void) {
    S_topBlock *p,top;
    log_trace("start new block");
    top = *s_topBlock;
    p = StackMemPush(&top, S_topBlock);
    // trail can't be reset to NULL, because in case of syntax errors
    // this would avoid balancing of } at the end of class
    /*& fillTopBlock(s_topBlock, s_topBlock->firstFreeIndex, tmpWorkMemoryi, NULL, p); */
    fillTopBlock(s_topBlock, s_topBlock->firstFreeIndex, tmpWorkMemoryi, s_topBlock->trail, p);
}

void stackMemoryBlockFree(void) {

    /*fprintf(dumpOut,"finish block\n");*/
    //&removeFromTrailUntil(NULL);
    assert(s_topBlock && s_topBlock->previousTopBlock);
    removeFromTrailUntil(s_topBlock->previousTopBlock->trail);
    /*fprintf(dumpOut,"block free %d %d \n",tmpWorkMemoryi,s_topBlock->tmpMemoryBasei); fflush(dumpOut);*/
    assert(tmpWorkMemoryi >= s_topBlock->tmpMemoryBasei);
    tmpWorkMemoryi = s_topBlock->tmpMemoryBasei;
    * s_topBlock =  * s_topBlock->previousTopBlock;
    /*  FILL_topBlock(s_topBlock,s_topBlock->firstFreeIndex,NULL,NULL); */
    // burk, following disables any memory freeing for Java
    //  if (LANGUAGE(LAN_JAVA)) s_topBlock->firstFreeIndex = memi;
    assert(s_topBlock != NULL);
}

void stackMemoryDump(void) {
    int i;
    fprintf(dumpOut,"start stackMemoryDump\n");
    fprintf(dumpOut,"sorry, not yet implemented ");
    for(i=0; i<s_topBlock->firstFreeIndex; i++) {
        if (i%10 == 0) fprintf(dumpOut," ");
        fprintf(dumpOut,".");
    }
    fprintf(dumpOut,"end stackMemoryDump\n");
}
