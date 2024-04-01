#include "classcaster.h"

#include "head.h"
#include "memory.h"
#include "log.h"


#define CCT_TREE_INDEX       4	/* number of subtrees on each cct node    */


/* ******************** Class Cast Tree ************************ */
/* Data structure storing for each class all classes it can be */
/* casted to */

static int cctTreeHash(Symbol *symbol, int depthFactor) {
    return (((long unsigned)symbol) >> 4)/(depthFactor)%CCT_TREE_INDEX;
}

protected void fillCctNode(CctNode *node, Symbol *symbol, CctNode *subtree) {
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
    log_trace("%*s%lx:%s",depth,"",(unsigned long)node->symbol,node->symbol->linkName);
    if (node->subtree == NULL)
        return;
    for (int i=0; i<CCT_TREE_INDEX; i++)
        cctDump(&node->subtree[i], depth+2);
}

#if 1
// TODO: make these into unit tests...
void cctTest(void) {
    CctNode node;
    fillCctNode(&node, NULL, NULL);
    Symbol symbol1 = {.linkName = "symbol1"};
    cctAddSimpleValue(&node, &symbol1, 1);
    cctDump(&node, 0);
    log_trace("----------\n");
    Symbol symbol2 = {.linkName = "symbol2"};
    cctAddSimpleValue(&node, &symbol2, 1);
    cctDump(&node, 0);
    log_trace("----------\n");
    Symbol symbol3 = {.linkName = "symbol3"};
    cctAddSimpleValue(&node, &symbol3, 1);
    cctDump(&node, 0);
    log_trace("----------\n");
    Symbol symbol4 = {.linkName = "symbol4"};
    cctAddSimpleValue(&node, &symbol4, 1);
    cctDump(&node, 0);
    log_trace("----------\n");

    CctNode top;
    fillCctNode(&top, NULL, NULL);
    cctDump(&top, 0);
    log_trace("----------\n");
    Symbol symbol5 = {.linkName = "symbol5"};
    cctAddSimpleValue(&top, &symbol5, 1);
    cctDump(&top, 0);
    log_trace("----------\n");
    Symbol symbol6 = {.linkName = "symbol6"};
    cctAddSimpleValue(&top, &symbol6, 1);
    cctDump(&top, 0);
    log_trace("----------\n");
    Symbol symbol7 = {.linkName = "symbol7"};
    cctAddSimpleValue(&top, &symbol7, 1);
    cctDump(&top, 0);
    log_trace("----------\n");
    Symbol symbol8 = {.linkName = "symbol8"};
    cctAddSimpleValue(&top, &symbol8, 1);
    cctDump(&top, 0);
    log_trace("----------\n");

    cctAddCctTree(&node, &top, 1);
    cctDump(&node, 0);
}
#endif
