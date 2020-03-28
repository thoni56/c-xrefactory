#ifndef CLASSH_H
#define CLASSH_H

#include "proto.h"

/* ************* class hierarchy  cross referencing ************** */

typedef struct chReference {
    int					ofile;		/* file of origin */
    int                 clas;		/* index of super-class */
    struct chReference	*next;
} S_chReference;



extern void classHierarchyGenInit(void);
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
