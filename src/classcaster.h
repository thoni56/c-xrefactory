#ifndef CLASSCASTTREE_H_INCLUDED
#define CLASSCASTTREE_H_INCLUDED

#include "symbol.h"

/* class cast tree */
typedef struct cctNode {
    struct symbol	*node;
    struct cctNode	*sub;       /* sub[CCT_TREE_INDEX]; */
} CctNode;


extern void cctAddSimpleValue(CctNode *cc, Symbol *x, int depthFactor);
extern bool cctIsMember(CctNode *cc, Symbol *x, int depthFactor);
extern void cctAddCctTree(CctNode *cc, CctNode *x, int depthFactor);
extern void cctDump(CctNode *cc, int depth);

#endif
