#include "browsingmenu.h"

#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "cxref.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "protocol.h"
#include "referenceableitem.h"


BrowsingMenu makeBrowsingMenu(ReferenceableItem referenceable, bool selected, bool visible, unsigned filterLevel,
                            char olUsage, char defaultUsage, Position defaultPosition) {
    BrowsingMenu menu;

    menu.referenceable = referenceable;
    menu.selected = selected;
    menu.visible = visible;
    menu.filterLevel = filterLevel;
    menu.olUsage = olUsage;
    menu.defaultUsage = defaultUsage;
    menu.defaultPosition = defaultPosition;

    /* Default values */
    menu.relation  = (SymbolRelation){.sameFile = false};
    menu.refn = 0;
    menu.defaultRefn = 0;
    menu.outOnLine = 0;
    menu.markers = NULL;
    menu.next = NULL;

    return menu;
}

static BrowsingMenu *freeBrowsingMenu(BrowsingMenu *menu) {
    free(menu->referenceable.linkName);
    freeReferences(menu->referenceable.references);
    BrowsingMenu *next = menu->next;
    free(menu);
    return next;
}


void freeBrowsingMenuList(BrowsingMenu *menuList) {
    BrowsingMenu *l;

    l = menuList;
    while (l != NULL) {
        l = freeBrowsingMenu(l);
    }
}

bool isBestFitMatch(BrowsingMenu *menu) {
    return menu->relation.sameFile;
}

void addReferenceToBrowsingMenu(BrowsingMenu *menu, Reference *reference) {
    Reference *added = addReferenceToList(reference, &menu->referenceable.references);
    if (added!=NULL) {
        if (isDefinitionOrDeclarationUsage(reference->usage)) {
            if (reference->usage==UsageDefined && positionsAreEqual(reference->position, menu->defaultPosition)) {
                added->usage = UsageOLBestFitDefined;
            }
            if (isMoreImportantUsageThan(reference->usage, menu->defaultUsage)) {
                menu->defaultUsage = reference->usage;
                menu->defaultPosition = reference->position;
            }
            menu->defaultRefn ++;
        } else {
            menu->refn ++;
        }
    }
}

void extendBrowsingMenuWithReferences(BrowsingMenu *menuItem, Reference *references) {
    for (Reference *r = references; r != NULL; r = r->next) {
        addReferenceToBrowsingMenu(menuItem, r);
    }
}

static void printGlobalReferenceLists(BrowsingMenu *menu, FILE *file);

void printSelectionMenu(BrowsingMenu *menu) {
    assert(options.xref2);

    ppcBegin(PPC_SYMBOL_RESOLUTION);
    if (menu!=NULL) {
        printGlobalReferenceLists(menu, outputFile);
    }
    ppcEnd(PPC_SYMBOL_RESOLUTION);
}

BrowsingMenu *createNewMenuItem(ReferenceableItem *referenceable, int includedFileNumber, Position defpos,
                               Usage defusage, bool selected, bool visible, unsigned filterLevel,
                               SymbolRelation relation, Usage olusage) {
    BrowsingMenu   *menu;
    char          *allocatedNameCopy;

    allocatedNameCopy = strdup(referenceable->linkName);

    ReferenceableItem item = makeReferenceableItem(allocatedNameCopy, referenceable->type, referenceable->storage, referenceable->scope,
                                                      referenceable->visibility, includedFileNumber);

    menu = malloc(sizeof(BrowsingMenu));
    *menu = makeBrowsingMenu(item, selected, visible, filterLevel, olusage, defusage, defpos);
    menu->relation = relation;
    return menu;
}

static bool referenceableItemIsLess(ReferenceableItem *item1, ReferenceableItem *item2) {
    int cmp;

    cmp = strcmp(item1->linkName, item2->linkName);
    if (cmp < 0)
        return true;
    else if (cmp > 0)
        return false;
    if (item1->includeFileNumber < item2->includeFileNumber)
        return true;
    else if (item1->includeFileNumber > item2->includeFileNumber)
        return false;
    if (item1->type < item2->type)
        return true;
    else if (item1->type > item2->type)
        return false;
    if (item1->storage < item2->storage)
        return true;
    else if (item1->storage > item2->storage)
        return false;
    if (item1->visibility < item2->visibility)
        return true;
    else if (item1->visibility > item2->visibility)
        return false;
    return false;
}

static bool browsingMenuIsLess(BrowsingMenu *s1, BrowsingMenu *s2) {
    return referenceableItemIsLess(&s1->referenceable, &s2->referenceable);
}

BrowsingMenu *addReferenceableToBrowsingMenu(BrowsingMenu **menuP, ReferenceableItem *referenceable, bool selected,
                                           bool visible, unsigned filterLevel,
                                           SymbolRelation relation, int olusage, Position defpos,
                                           int defusage) {
    BrowsingMenu **place;

    BrowsingMenu dummyMenu = makeBrowsingMenu(*referenceable, 0, false, 0, olusage, UsageNone,
                                            NO_POSITION);
    SORTED_LIST_PLACE3(place, BrowsingMenu, &dummyMenu, menuP, browsingMenuIsLess);

    BrowsingMenu *new = *place;
    if (*place==NULL || browsingMenuIsLess(&dummyMenu, *place)) {
        assert(referenceable);
        new = createNewMenuItem(referenceable, referenceable->includeFileNumber, defpos, defusage,
                                selected, visible, filterLevel, relation, olusage);
        LIST_CONS(new, *place);
        log_debug(":adding browsed symbol '%s'", referenceable->linkName);
    }
    return new;
}

static int currentOutputLineInSymbolList =0;


static void printMenuItemPrefix(FILE *file, BrowsingMenu *menu, bool selectable) {
    if (! selectable) {
        fprintf(file, " %s=2", PPCA_SELECTED);
    } else if (menu!=NULL && menu->selected) {
        fprintf(file, " %s=1", PPCA_SELECTED);
    } else {
        fprintf(file, " %s=0", PPCA_SELECTED);
    }

    fprintf(file, " base=0");   /* Legacy value */

    if (menu==NULL || (menu->defaultRefn==0 && menu->refn==0) || !selectable) {
        fprintf(file, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
    } else if (menu->defaultRefn==0) {
        fprintf(file, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, menu->refn);
    } else if (menu->refn==0) {
        fprintf(file, " %s=%d %s=0", PPCA_DEF_REFN, menu->defaultRefn, PPCA_REFN);
    } else {
        fprintf(file, " %s=%d %s=%d", PPCA_DEF_REFN, menu->defaultRefn, PPCA_REFN, menu->refn);
    }
}

static void printMenuEntry(FILE *file, BrowsingMenu *menu) {

    if (currentOutputLineInSymbolList == 1)
        currentOutputLineInSymbolList++; // first line irregularity
    menu->outOnLine = currentOutputLineInSymbolList;
    currentOutputLineInSymbolList++ ;
    ppcIndent();
    fprintf(file,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
    if (menu->referenceable.type!=TypeDefault) {
        fprintf(file," %s=%s", PPCA_TYPE, typeNamesTable[menu->referenceable.type]);
    }
    printMenuItemPrefix(file, menu, true);

    char tempString[MAX_CX_SYMBOL_SIZE];
    prettyPrintLinkNameForSymbolInMenu(tempString, menu);
    fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
}

/* Mapped through 'splitMenuPerSymbolsAndMap()' */
static void printReferenceListsForMenuEntries(BrowsingMenu *menu, void *p1) {
    FILE *file = (FILE *)p1;

    // Are there are any visible references at all
    BrowsingMenu *m;
    for (m=menu; m!=NULL && !m->visible; m=m->next)
        ;
    if (m == NULL)
        return;

    for (BrowsingMenu *m=menu; m!=NULL; m=m->next) {
        printMenuEntry(file, m);
    }
}

/* Groups menu items by symbol (ReferenceableItem), applies function to each group,
 * then reconstructs the original list with the original head preserved.
 * This allows processing all variants of the same symbol together. */
void splitBrowsingMenuAndMap(BrowsingMenu *menu, void (*fun)(BrowsingMenu *, void *), void *p1) {
    BrowsingMenu *current, *group, **listPtr, *candidate, *reconstructedList;
    ReferenceableItem *currentItem;

    reconstructedList = NULL;
    current = menu;

    while (current != NULL) {
        /* Start a new group for the current symbol */
        group = NULL;
        listPtr = &current;
        currentItem = &current->referenceable;

        /* Collect all menu items matching this symbol into 'group' */
        while (*listPtr != NULL) {
            candidate = *listPtr;
            if (isSameReferenceableItem(&candidate->referenceable, currentItem)) {
                /* Remove candidate from main list and prepend to group */
                *listPtr = candidate->next;
                candidate->next = group;
                group = candidate;
            } else {
                /* Move to next item in main list */
                listPtr = &(*listPtr)->next;
            }
        }

        /* Process this group of items for the same symbol */
        (*fun)(group, p1);

        /* Append the processed group to the reconstructed list */
        LIST_APPEND(BrowsingMenu, group, reconstructedList);
        reconstructedList = group;
    }

    /* Restore the original head of the list to maintain expected ordering */
    if (reconstructedList != menu) {
        listPtr = &reconstructedList;
        while (*listPtr != menu && *listPtr != NULL)
            listPtr = &(*listPtr)->next;
        assert(*listPtr != NULL);
        assert(*listPtr != reconstructedList);
        *listPtr = menu->next;
        menu->next = reconstructedList;
    }
}

static void printGlobalReferenceLists(BrowsingMenu *menu, FILE *file) {
    for (BrowsingMenu *m=menu; m!=NULL; m=m->next)
        m->outOnLine = 0;
    currentOutputLineInSymbolList = 1;
    splitBrowsingMenuAndMap(menu, printReferenceListsForMenuEntries, file);
}
