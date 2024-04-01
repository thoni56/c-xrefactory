#include "classcaster.h"

#include "memory.h"
#include "log.h"


#define CCT_TREE_INDEX       4	/* number of subtrees on each cct node    */


/* ******************** Class Cast Tree ************************ */
/* Data structure storing for each class all classes it can be */
/* casted to */

static int cctTreeHash(Symbol *symbol, int depthFactor) {
    return (((long unsigned)symbol) >> 4)/(depthFactor)%CCT_TREE_INDEX;
}

static void fillCctNode(CctNode *cct, Symbol *node, CctNode *subtree) {
    cct->node = node;
    cct->sub = subtree;
}

void cctAddSimpleValue(CctNode *cc, Symbol *symbol, int depthFactor) {
    CctNode *nn;
    int i,h;

    log_trace("adding %d == %s to casts at %d at depth %d", symbol, symbol->linkName, cc, depthFactor);
    if (cc->node == symbol)
        return;
    if (cc->node == NULL) {
        cc->node = symbol;
        return;
    }
    h = cctTreeHash(symbol, depthFactor);
    if (cc->sub == NULL) {
        CF_ALLOCC(nn, CCT_TREE_INDEX, CctNode);
        for(i=0; i<CCT_TREE_INDEX; i++) fillCctNode(&nn[i], NULL, NULL);
        nn[h].node = symbol;
        cc->sub = nn;
        return;
    }
    cctAddSimpleValue(&cc->sub[h], symbol, depthFactor*CCT_TREE_INDEX);
}


bool cctIsMember(CctNode *cc, Symbol *symbol, int depthFactor) {
    int h,result;
    //&if (cc->node!=NULL) fprintf(dumpOut,"checking cast %s to %d == %s\n", cc->node->linkName, symbol, symbol->linkName);
    if (cc->node == symbol) return true;
    if (cc->sub == NULL) return false;
    h = cctTreeHash(symbol, depthFactor);
    assert(h>=0 && h<CCT_TREE_INDEX);
    result = cctIsMember(&cc->sub[h], symbol, depthFactor*CCT_TREE_INDEX);
    return result;
}


void cctAddCctTree(CctNode *cc, CctNode *x, int depthFactor) {
    int i;

    log_trace("adding %d tree to %d tree at depth == %d", x, cc, depthFactor);
    if (x->node == NULL) return;
    if (cc->node == NULL) {
        *cc = *x;
        return;
    }
    cctAddSimpleValue(cc, x->node, depthFactor);
    if (x->sub == NULL) return;
    if (cc->sub == NULL) {
        CF_ALLOCC(cc->sub, CCT_TREE_INDEX, CctNode);
        for(i=0; i<CCT_TREE_INDEX; i++) cc->sub[i] = x->sub[i];
    } else {
        for(i=0; i<CCT_TREE_INDEX; i++) {
            cctAddCctTree(&cc->sub[i], &x->sub[i], depthFactor*CCT_TREE_INDEX);
        }
    }
}

void cctDump(CctNode *cc, int depth) {
    int i;
    if (cc->node==NULL) {
        log_trace("%*sNULL",depth,"");
        return;
    }
    log_trace("%*s%lx",depth,"",(unsigned long)cc->node);
    if (cc->sub == NULL) return;
    for(i=0; i<CCT_TREE_INDEX; i++) cctDump(&cc->sub[i], depth+2);
}

#if 0
// TODO: make these into unit tests...
void cctTest(void) {
    S_cctNode nn,hh;
    fillCctNode(&nn, NULL, NULL);
    cctDump(&nn, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&nn, 0x10, 1);
    cctDump(&nn, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&nn, 0x20, 1);
    cctDump(&nn, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&nn, 0x30, 1);
    cctDump(&nn, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&nn, 0xa0, 1);
    cctDump(&nn, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);

    fillCctNode(&hh, NULL, NULL);
    cctDump(&hh, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&hh, 0x50, 1);
    cctDump(&hh, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&hh, 0x20, 1);
    cctDump(&hh, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&hh, 0x40, 1);
    cctDump(&hh, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    cctAddSimpleValue(&hh, 0xe0, 1);
    cctDump(&hh, 0);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);
    fprintf(dumpOut,"----------\n"); fflush(dumpOut);

    cctAddCctTree(&nn, &hh, 1);
    cctDump(&nn, 0);
}
#endif
