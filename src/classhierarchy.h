#ifndef _CLASSHIERARCHY_H_
#define _CLASSHIERARCHY_H_

#include "proto.h"

/* ************* class hierarchy cross referencing ************** */

typedef struct classHierarchyReference {
    int					ofile;		/* file of origin */
    int                 superClass;		/* index of super-class */
    struct classHierarchyReference	*next;
} ClassHierarchyReference;



extern ClassHierarchyReference *newClassHierarchyReference(int origin, int class, ClassHierarchyReference *next);
extern void classHierarchyGenInit(void);
extern void setTmpClassBackPointersToMenu(S_olSymbolsMenu *menu);
extern void splitMenuPerSymbolsAndMap(S_olSymbolsMenu *rrr,
                                      void (*fun)(S_olSymbolsMenu *, void *, void *),
                                      void *p1,
                                      char *p2
                                      );
extern void htmlGenerateGlobalReferenceLists(S_olSymbolsMenu *rrr, FILE *ff, char *fn);
extern void genClassHierarchies(FILE *ff, S_olSymbolsMenu *rrr,
                                int virtFlag, int pass );
extern int classHierarchyClassNameLess(int c1, int c2);
extern int classHierarchySupClassNameLess(ClassHierarchyReference *c1, ClassHierarchyReference *c2);


#endif
