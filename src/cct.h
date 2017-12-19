#ifndef CCT_H
#define CCT_H

#include "proto.h"

extern void cctAddSimpleValue(S_cctNode *cc, S_symbol *x, int deepFactor);
extern int cctIsMember(S_cctNode *cc, S_symbol *x, int deepFactor);
extern void cctAddCctTree(S_cctNode *cc, S_cctNode *x, int deepFactor);
extern void cctDump(S_cctNode *cc, int deep);
extern void cctTest();

#endif
