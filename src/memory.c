#include "globals.h"

#include "memory.h"
#include "strFill.h"
#include "log.h"
#include "commons.h"

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
