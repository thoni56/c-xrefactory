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


extern void fillSymbolsMenu(SymbolsMenu *menu, struct referencesItem s, bool selected, bool visible,
                            unsigned ooBits, char olUsage, short int vlevel,
                            char defUsage, struct position defpos);

extern SymbolsMenu *freeSymbolsMenu(SymbolsMenu *menu);
extern void freeSymbolsMenuList(SymbolsMenu *menu);
extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference, int bestFitFlag);
extern void olcxPrintClassTree(SymbolsMenu *menu);
extern void olcxPrintSelectionMenu(SymbolsMenu *menu);
extern SymbolsMenu *olCreateNewMenuItem(ReferenceItem *sym, int vApplClass, int vFunCl,
                                        Position *defpos, int defusage, int selected, int visible,
                                        unsigned ooBits, int olusage, int vlevel);
extern SymbolsMenu *olAddBrowsedSymbolToMenu(SymbolsMenu **menuP, ReferenceItem *reference,
                                             bool selected, bool visible, unsigned ooBits,
                                             int olusage, int vlevel,
                                             Position *defpos, int defusage);

#endif
