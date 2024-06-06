#ifndef CLASSHIERARCHY_H_INCLUDED
#define CLASSHIERARCHY_H_INCLUDED

#include <stdio.h>

#include "menu.h"


extern void splitMenuPerSymbolsAndMap(SymbolsMenu *menu,
                                      void (*fun)(SymbolsMenu *, void *),
                                      void *p1);
extern void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file);

#endif
