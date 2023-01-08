#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include "reference.h"

typedef struct SymbolsMenu {
    struct referencesItem    references;
    bool                     selected;
    bool                     visible;
    unsigned                 ooBits;
    char                     olUsage; /* usage of symbol under cursor */
    short int                vlevel;  /* virt. level of applClass <-> olsymbol*/
    short int                refn;
    short int                defRefn;
    char                     defUsage; /* usage of definition reference */
    struct position          defpos;
    int                      outOnLine;
    struct editorMarkerList *markers; /* for refactory only */
    struct SymbolsMenu      *next;
} SymbolsMenu;


extern void fillSymbolsMenu(SymbolsMenu *symbolsMenu, struct referencesItem s, bool selected, bool visible,
                            unsigned ooBits, char olUsage, short int vlevel,
                            char defUsage, struct position defpos);

extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference, int bestFitFlag);

#endif
