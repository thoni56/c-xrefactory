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



extern void splitMenuPerSymbolsAndMap(SymbolsMenu *menu,
                                      void (*fun)(SymbolsMenu *, void *),
                                      void *p1);
extern void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file);

#endif
