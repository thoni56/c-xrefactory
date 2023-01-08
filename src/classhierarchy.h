#ifndef CLASSHIERARCHY_H_INCLUDED
#define CLASSHIERARCHY_H_INCLUDED

#include <stdio.h>

#include "menu.h"


/* ************* class hierarchy cross referencing ************** */

typedef struct classHierarchyReference {
    int					ofile;		/* file of origin */
    int                 superClass;		/* index of super-class */
    struct classHierarchyReference	*next;
} ClassHierarchyReference;



extern ClassHierarchyReference *newClassHierarchyReference(int origin, int superClass,
                                                           ClassHierarchyReference *next);
extern void initClassHierarchyGeneration(void);
extern void setTmpClassBackPointersToMenu(SymbolsMenu *menu);
extern void splitMenuPerSymbolsAndMap(SymbolsMenu *menu,
                                      void (*fun)(SymbolsMenu *, void *, void *),
                                      void *p1,
                                      char *p2
                                      );
extern void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file, char *fn);
extern void genClassHierarchies(SymbolsMenu *menu, FILE *file, int pass);
extern bool isSmallerOrEqClass(int inferior, int superior);
extern int classCmp(int class1, int class2);
extern bool classHierarchyClassNameLess(int c1, int c2);

#endif
