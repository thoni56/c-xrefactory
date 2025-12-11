#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include "referenceableitem.h"

typedef struct {
    bool sameFile;
} SymbolRelation;

typedef struct SymbolsMenu {
    struct referenceableItem    references;
    bool                     selected;
    bool                     visible;
    unsigned                 ooBits;
    SymbolRelation           relation;
    char                     olUsage; /* usage of symbol under cursor */
    short int                vlevel;  /* virt. level of applClass <-> olsymbol*/
    short int                refn;
    short int                defaultRefn;
    char                     defaultUsage; /* usage of definition reference */
    struct position          defaultPosition;
    int                      outOnLine;
    struct editorMarkerList *markers; /* for refactory only */
    struct SymbolsMenu      *next;
} SymbolsMenu;


extern SymbolsMenu makeSymbolsMenu(ReferenceableItem references, bool selected, bool visible,
                                   unsigned ooBits, char olUsage, short int vlevel,
                                   char defUsage, struct position defpos);

extern void freeSymbolsMenuList(SymbolsMenu *menu);

extern bool isBestFitMatch(SymbolsMenu *menu);
extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference);
extern void olcxPrintSelectionMenu(SymbolsMenu *menu);
extern SymbolsMenu *createNewMenuItem(ReferenceableItem *sym, int includedFileNumber,
                                      Position defpos, Usage defusage, bool selected, bool visible,
                                      unsigned ooBits, SymbolRelation relation, Usage olusage, int vlevel);
extern SymbolsMenu *addBrowsedSymbolToMenu(SymbolsMenu **menuP, ReferenceableItem *reference,
                                           bool selected, bool visible, unsigned ooBits, SymbolRelation relation,
                                           int olusage, int vlevel, Position defpos, int defusage);
extern void splitMenuPerSymbolsAndMap(SymbolsMenu *menu,
                                      void (*fun)(SymbolsMenu *, void *),
                                      void *p1);

#endif
