#ifndef CLASSH_H
#define CLASSH_H

#include "proto.h"

extern void classHierarchyGenInit();
extern void setTmpClassBackPointersToMenu(S_olSymbolsMenu *menu);
extern void splitMenuPerSymbolsAndMap(
                                      S_olSymbolsMenu *rrr,
                                      void (*fun)(S_olSymbolsMenu *, void *, void *),
                                      void *p1,
                                      char *p2
                                      );
extern void htmlGenGlobRefLists(S_olSymbolsMenu *rrr, FILE *ff, char *fn);
extern void genClassHierarchies(FILE *ff, S_olSymbolsMenu *rrr,
                                int virtFlag, int pass );
extern int classHierarchyClassNameLess(int c1, int c2);
extern int classHierarchySupClassNameLess(S_chReference *c1, S_chReference *c2);


#endif
