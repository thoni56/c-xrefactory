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

static void fillCctNode(CctNode *node, Symbol *symbol, CctNode *subtree) {
    node->symbol = symbol;
    node->subtree = subtree;
}

void cctAddSimpleValue(CctNode *node, Symbol *symbol, int depthFactor) {
    log_trace("adding %d == %s to casts at %d at depth %d", symbol, symbol->linkName, node, depthFactor);
    if (node->symbol == symbol)
        return;
    if (node->symbol == NULL) {
        node->symbol = symbol;
        return;
    }
    int hash = cctTreeHash(symbol, depthFactor);
    if (node->subtree == NULL) {
        CctNode *n;
        CF_ALLOCC(n, CCT_TREE_INDEX, CctNode);
        for (int i=0; i<CCT_TREE_INDEX; i++)
            fillCctNode(&n[i], NULL, NULL);
        n[hash].symbol = symbol;
        node->subtree = n;
        return;
    }
    cctAddSimpleValue(&node->subtree[hash], symbol, depthFactor*CCT_TREE_INDEX);
}


bool cctIsMember(CctNode *node, Symbol *symbol, int depthFactor) {
    if (node->symbol == symbol)
        return true;
    if (node->subtree == NULL)
        return false;
    int hash = cctTreeHash(symbol, depthFactor);
    assert(hash>=0 && hash<CCT_TREE_INDEX);
    return cctIsMember(&node->subtree[hash], symbol, depthFactor*CCT_TREE_INDEX);
}


void cctAddCctTree(CctNode *node, CctNode *symbol, int depthFactor) {
    log_trace("adding %d tree to %d tree at depth == %d", symbol, node, depthFactor);
    if (symbol->symbol == NULL)
        return;
    if (node->symbol == NULL) {
        *node = *symbol;
        return;
    }
    cctAddSimpleValue(node, symbol->symbol, depthFactor);
    if (symbol->subtree == NULL)
        return;
    if (node->subtree == NULL) {
        CF_ALLOCC(node->subtree, CCT_TREE_INDEX, CctNode);
        for (int i=0; i<CCT_TREE_INDEX; i++)
            node->subtree[i] = symbol->subtree[i];
    } else {
        for (int i=0; i<CCT_TREE_INDEX; i++)
            cctAddCctTree(&node->subtree[i], &symbol->subtree[i], depthFactor*CCT_TREE_INDEX);
    }
}

void cctDump(CctNode *node, int depth) {
    if (node->symbol==NULL) {
        log_trace("%*sNULL",depth,"");
        return;
    }
    log_trace("%*s%lx",depth,"",(unsigned long)node->symbol);
    if (node->subtree == NULL)
        return;
    for (int i=0; i<CCT_TREE_INDEX; i++)
        cctDump(&node->subtree[i], depth+2);
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
