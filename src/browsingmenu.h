#ifndef BROWSINGMENU_H_INCLUDED
#define BROWSINGMENU_H_INCLUDED

#include "referenceableitem.h"

typedef struct {
    bool sameFile;
} SymbolRelation;

typedef struct BrowsingMenu {
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
    struct BrowsingMenu      *next;
} BrowsingMenu;


extern BrowsingMenu makeBrowsingMenu(ReferenceableItem referenceable, bool selected, bool visible,
                                   unsigned filterLevel, char olUsage,
                                   char defUsage, struct position defpos);

extern void freeBrowsingMenuList(BrowsingMenu *menu);

extern bool isBestFitMatch(BrowsingMenu *menu);
extern void addReferenceToBrowsingMenu(BrowsingMenu *menu, Reference *reference);
extern void printSelectionMenu(BrowsingMenu *menu);
extern BrowsingMenu *createNewMenuItem(ReferenceableItem *item, int includedFileNumber,
                                      Position defpos, Usage defusage, bool selected, bool visible,
                                      unsigned filterLevel, SymbolRelation relation, Usage olusage);
extern BrowsingMenu *addReferenceableToBrowsingMenu(BrowsingMenu **menuP, ReferenceableItem *referenceable,
                                                  bool selected, bool visible, unsigned filterLevel,
                                                  SymbolRelation relation, int olusage, Position defpos,
                                                  int defusage);
extern void splitBrowsingMenuAndMap(BrowsingMenu *menu, void (*fun)(BrowsingMenu *, void *), void *p1);
extern void extendBrowsingMenuWithReferences(BrowsingMenu *menuItem, Reference *references);

#endif
