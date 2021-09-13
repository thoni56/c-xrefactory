#ifndef CLASSHIERARCHY_H_INCLUDED
#define CLASSHIERARCHY_H_INCLUDED

#include "proto.h"

/* ************* class hierarchy cross referencing ************** */

typedef struct classHierarchyReference {
    int					ofile;		/* file of origin */
    int                 superClass;		/* index of super-class */
    struct classHierarchyReference	*next;
} ClassHierarchyReference;



extern ClassHierarchyReference *newClassHierarchyReference(int origin, int class, ClassHierarchyReference *next);
extern void classHierarchyGenInit(void);
extern void setTmpClassBackPointersToMenu(SymbolsMenu *menu);
extern void splitMenuPerSymbolsAndMap(SymbolsMenu *rrr,
                                      void (*fun)(SymbolsMenu *, void *, void *),
                                      void *p1,
                                      char *p2
                                      );
extern void generateGlobalReferenceLists(SymbolsMenu *rrr, FILE *ff, char *fn);
extern void genClassHierarchies(FILE *ff, SymbolsMenu *rrr,
                                int virtFlag, int pass );
extern bool classHierarchyClassNameLess(int c1, int c2);
extern int classHierarchySupClassNameLess(ClassHierarchyReference *c1, ClassHierarchyReference *c2);


#endif
