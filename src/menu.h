#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include "reference.h"


typedef struct SymbolsMenu {
    struct referenceItem    references;
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


extern SymbolsMenu makeSymbolsMenu(ReferenceItem references, bool selected, bool visible,
                                   unsigned ooBits, char olUsage, short int vlevel,
                                   char defUsage, struct position defpos);

extern void freeSymbolsMenuList(SymbolsMenu *menu);
extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference);
extern void olcxPrintSelectionMenu(SymbolsMenu *menu);
extern SymbolsMenu *createNewMenuItem(ReferenceItem *sym, int includedFileNumber,
                                        Position defpos, int defusage, int selected, int visible,
                                        unsigned ooBits, Usage olusage, int vlevel);
extern SymbolsMenu *addBrowsedSymbolToMenu(SymbolsMenu **menuP, ReferenceItem *reference,
                                             bool selected, bool visible, unsigned ooBits, int olusage,
                                             int vlevel, Position defpos, int defusage);
extern void splitMenuPerSymbolsAndMap(SymbolsMenu *menu,
                                      void (*fun)(SymbolsMenu *, void *),
                                      void *p1);

#endif
