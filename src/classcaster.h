#ifndef CLASSCASTTREE_H_INCLUDED
#define CLASSCASTTREE_H_INCLUDED

#include "symbol.h"

/* class cast tree */
typedef struct cctNode {
    struct symbol	*symbol;
    struct cctNode	*subtree;       /* sub[CCT_TREE_INDEX]; */
} CctNode;


extern void cctAddSimpleValue(CctNode *node, Symbol *symbol, int depthFactor);
extern bool cctIsMember(CctNode *node, Symbol *symbol, int depthFactor);
extern void cctAddCctTree(CctNode *node, CctNode *tree, int depthFactor);
extern void cctDump(CctNode *node, int depth);

#endif
