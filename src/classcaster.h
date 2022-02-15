#ifndef CLASSCASTTREE_H_INCLUDED
#define CLASSCASTTREE_H_INCLUDED

#include "symbol.h"

/* class cast tree */
typedef struct cctNode {
    struct symbol	*node;
    struct cctNode	*sub;       /* sub[CCT_TREE_INDEX]; */
} S_cctNode;


extern void cctAddSimpleValue(S_cctNode *cc, Symbol *x, int depthFactor);
extern bool cctIsMember(S_cctNode *cc, Symbol *x, int depthFactor);
extern void cctAddCctTree(S_cctNode *cc, S_cctNode *x, int depthFactor);
extern void cctDump(S_cctNode *cc, int depth);

#endif
