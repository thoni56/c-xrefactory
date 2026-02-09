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
    unsigned                 filterLevel;
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


extern BrowserMenu makeBrowserMenu(ReferenceableItem referenceable, bool selected, bool visible,
                                   unsigned filterLevel, char olUsage,
                                   char defUsage, struct position defpos);

extern void freeBrowserMenuList(BrowserMenu *menu);

extern bool isBestFitMatch(BrowserMenu *menu);
extern void addReferenceToBrowserMenu(BrowserMenu *menu, Reference *reference);
extern void printSelectionMenu(BrowserMenu *menu);
extern BrowserMenu *createNewMenuItem(ReferenceableItem *item, int includedFileNumber,
                                      Position defpos, Usage defusage, bool selected, bool visible,
                                      unsigned filterLevel, SymbolRelation relation, Usage olusage);
extern BrowserMenu *addReferenceableToBrowserMenu(BrowserMenu **menuP, ReferenceableItem *referenceable,
                                                  bool selected, bool visible, unsigned filterLevel,
                                                  SymbolRelation relation, int olusage, Position defpos,
                                                  int defusage);
extern void splitBrowserMenuAndMap(BrowserMenu *menu, void (*fun)(BrowserMenu *, void *), void *p1);
extern void extendBrowserMenuWithReferences(BrowserMenu *menuItem, Reference *references);

#endif
