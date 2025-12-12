#ifndef BROWSERMENU_H_INCLUDED
#define BROWSERMENU_H_INCLUDED

#include "referenceableitem.h"

typedef struct {
    bool sameFile;
} SymbolRelation;

typedef struct BrowserMenu {
    struct referenceableItem referenceable;
    bool                     selected;
    bool                     visible;
    unsigned                 ooBits;
    SymbolRelation           relation;
    char                     olUsage; /* usage of symbol under cursor */
    short int                refn;
    short int                defaultRefn;
    char                     defaultUsage; /* usage of definition reference */
    struct position          defaultPosition;
    int                      outOnLine;
    struct editorMarkerList *markers; /* for refactory only */
    struct BrowserMenu      *next;
} BrowserMenu;


extern BrowserMenu makeBrowserMenu(ReferenceableItem referenceables, bool selected, bool visible,
                                   unsigned ooBits, char olUsage,
                                   char defUsage, struct position defpos);

extern void freeBrowserMenuList(BrowserMenu *menu);

extern bool isBestFitMatch(BrowserMenu *menu);
extern void addReferenceToBrowserMenu(BrowserMenu *menu, Reference *reference);
extern void olcxPrintSelectionMenu(BrowserMenu *menu);
extern BrowserMenu *createNewMenuItem(ReferenceableItem *sym, int includedFileNumber,
                                      Position defpos, Usage defusage, bool selected, bool visible,
                                      unsigned ooBits, SymbolRelation relation, Usage olusage);
extern BrowserMenu *addReferenceableToBrowserMenu(BrowserMenu **menuP, ReferenceableItem *reference,
                                           bool selected, bool visible, unsigned ooBits, SymbolRelation relation,
                                           int olusage, Position defpos, int defusage);
extern void splitBrowserMenuAndMap(BrowserMenu *menu,
                                      void (*fun)(BrowserMenu *, void *),
                                      void *p1);

#endif
