#include "cct.h"

#include "commons.h"
#include "globals.h"
#include "log.h"


/* ******************** Class Cast Tree ************************ */
/* Data structure storing for each class all classes it can be */
/* casted to */

#define CCT_TREE_HASH(xx,deepFactor) ((((long unsigned)xx) >> 4)/(deepFactor)%CCT_TREE_INDEX)

static void fillCctNode(S_cctNode *cct, Symbol *node, S_cctNode *subtree) {
    cct->node = node;
    cct->sub = subtree;
}

void cctAddSimpleValue(S_cctNode *cc, Symbol *x, int deepFactor) {
    S_cctNode *nn;
    int i,h;

    log_trace("adding %d == %s to casts at %d at deep %d", x, x->linkName, cc, deepFactor);
    if (cc->node == x) return;
    if (cc->node == NULL) {
        cc->node = x;           /* should be trailed ? */
        return;
    }
    h = CCT_TREE_HASH(x, deepFactor);
    if (cc->sub == NULL) {
        CF_ALLOCC(nn, CCT_TREE_INDEX, S_cctNode);
        for(i=0; i<CCT_TREE_INDEX; i++) fillCctNode(&nn[i], NULL, NULL);
        nn[h].node = x;
        cc->sub = nn;           /* should be trailed ? */
        return;
    }
    cctAddSimpleValue(&cc->sub[h], x, deepFactor*CCT_TREE_INDEX);
}


int cctIsMember(S_cctNode *cc, Symbol *x, int deepFactor) {
    int h,res;
    //&if (cc->node!=NULL) fprintf(dumpOut,"checking cast %s to %d == %s\n", cc->node->linkName, x, x->linkName);
    if (cc->node == x) return(1);
    if (cc->sub == NULL) return(0);
    h = CCT_TREE_HASH(x, deepFactor);
    assert(h>=0 && h<CCT_TREE_INDEX);
    res = cctIsMember(&cc->sub[h], x, deepFactor*CCT_TREE_INDEX);
    return(res);
}


void cctAddCctTree(S_cctNode *cc, S_cctNode *x, int deepFactor) {
    int i;

    log_trace("adding %d tree to %d tree at deep == %d", x, cc, deepFactor);
    if (x->node == NULL) return;
    if (cc->node == NULL) {
        *cc = *x;           /* should be trailed ? */
        return;
    }
    cctAddSimpleValue(cc, x->node, deepFactor);
    if (x->sub == NULL) return;
    if (cc->sub == NULL) {
        CF_ALLOCC(cc->sub, CCT_TREE_INDEX, S_cctNode);
        for(i=0; i<CCT_TREE_INDEX; i++) cc->sub[i] = x->sub[i];
    } else {
        for(i=0; i<CCT_TREE_INDEX; i++) {
            cctAddCctTree(&cc->sub[i], &x->sub[i], deepFactor*CCT_TREE_INDEX);
        }
    }
}

void cctDump(S_cctNode *cc, int deep) {
    int i;
    if (cc->node==NULL) {
        log_trace("%*sNULL",deep,"");
        return;
    }
    log_trace("%*s%lx",deep,"",(unsigned long)cc->node);
    if (cc->sub == NULL) return;
    for(i=0; i<CCT_TREE_INDEX; i++) cctDump(&cc->sub[i], deep+2);
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
