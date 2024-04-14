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
        CctNode *n = cfAllocc(CCT_TREE_INDEX, CctNode);
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
        node->subtree = cfAllocc(CCT_TREE_INDEX, CctNode);
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
