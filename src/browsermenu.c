#include "browsermenu.h"

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
#include "referenceableitem.h"


BrowserMenu makeBrowserMenu(ReferenceableItem referenceable, bool selected, bool visible, unsigned ooBits,
                            char olUsage, short int vlevel, char defaultUsage, Position defaultPosition) {
    BrowserMenu menu;

    menu.referenceable = referenceable;
    menu.selected   = selected;
    menu.visible    = visible;
    menu.ooBits     = ooBits;
    menu.olUsage    = olUsage;
    menu.vlevel     = vlevel;
    menu.defaultUsage    = defaultUsage;
    menu.defaultPosition = defaultPosition;

    /* Default values */
    menu.relation  = (SymbolRelation){.sameFile = false};
    menu.refn      = 0;
    menu.defaultRefn   = 0;
    menu.outOnLine = 0;
    menu.markers   = NULL;
    menu.next      = NULL;

    return menu;
}

static BrowserMenu *freeBrowserMenu(BrowserMenu *menu) {
    free(menu->referenceable.linkName);
    freeReferences(menu->referenceable.references);
    BrowserMenu *next = menu->next;
    free(menu);
    return next;
}


void freeBrowserMenuList(BrowserMenu *menuList) {
    BrowserMenu *l;

    l = menuList;
    while (l != NULL) {
        l = freeBrowserMenu(l);
    }
}

bool isBestFitMatch(BrowserMenu *menu) {
    return menu->relation.sameFile;
}

void addReferenceToBrowserMenu(BrowserMenu *menu, Reference *reference) {
    Reference *added = addReferenceToList(&menu->referenceable.references, reference);
    if (added!=NULL) {
        if (isDefinitionOrDeclarationUsage(reference->usage)) {
            if (reference->usage==UsageDefined && positionsAreEqual(reference->position, menu->defaultPosition)) {
                added->usage = UsageOLBestFitDefined;
            }
            if (reference->usage < menu->defaultUsage) {
                menu->defaultUsage = reference->usage;
                menu->defaultPosition = reference->position;
            }
            menu->defaultRefn ++;
        } else {
            menu->refn ++;
        }
    }
}

static void generateGlobalReferenceLists(BrowserMenu *menu, FILE *file);

void olcxPrintSelectionMenu(BrowserMenu *menu) {
    assert(options.xref2);

    ppcBegin(PPC_SYMBOL_RESOLUTION);
    if (menu!=NULL) {
        generateGlobalReferenceLists(menu, outputFile);
    }
    ppcEnd(PPC_SYMBOL_RESOLUTION);
}

BrowserMenu *createNewMenuItem(ReferenceableItem *item, int includedFileNumber, Position defpos,
                               Usage defusage, bool selected, bool visible, unsigned ooBits,
                               SymbolRelation relation, Usage olusage, int vlevel) {
    BrowserMenu   *menu;
    char          *allocatedNameCopy;

    allocatedNameCopy = strdup(item->linkName);

    ReferenceableItem refItem = makeReferenceableItem(allocatedNameCopy, item->type, item->storage, item->scope,
                                                      item->visibility, includedFileNumber);

    menu = malloc(sizeof(BrowserMenu));
    *menu = makeBrowserMenu(refItem, selected, visible, ooBits, olusage, vlevel, defusage, defpos);
    menu->relation = relation;
    return menu;
}

static bool referenceableItemIsLess(ReferenceableItem *s1, ReferenceableItem *s2) {
    int cmp;

    cmp = strcmp(s1->linkName, s2->linkName);
    if (cmp < 0)
        return true;
    else if (cmp > 0)
        return false;
    if (s1->includeFile < s2->includeFile)
        return true;
    else if (s1->includeFile > s2->includeFile)
        return false;
    if (s1->type < s2->type)
        return true;
    else if (s1->type > s2->type)
        return false;
    if (s1->storage < s2->storage)
        return true;
    else if (s1->storage > s2->storage)
        return false;
    if (s1->visibility < s2->visibility)
        return true;
    else if (s1->visibility > s2->visibility)
        return false;
    return false;
}

static bool browserMenuIsLess(BrowserMenu *s1, BrowserMenu *s2) {
    return referenceableItemIsLess(&s1->referenceable, &s2->referenceable);
}

BrowserMenu *addReferenceableToBrowserMenu(BrowserMenu **menuP, ReferenceableItem *item,
                                  bool selected, bool visible, unsigned ooBits, SymbolRelation relation,
                                  int olusage, int vlevel, Position defpos, int defusage) {
    BrowserMenu **place;

    BrowserMenu dummyMenu = makeBrowserMenu(*item, 0, false, 0, olusage, vlevel, UsageNone, noPosition);
    SORTED_LIST_PLACE3(place, BrowserMenu, &dummyMenu, menuP, browserMenuIsLess);

    BrowserMenu *new = *place;
    if (*place==NULL || browserMenuIsLess(&dummyMenu, *place)) {
        assert(item);
        new = createNewMenuItem(item, item->includeFile, defpos, defusage,
                                selected, visible, ooBits, relation, olusage, vlevel);
        LIST_CONS(new, *place);
        log_debug(":adding browsed symbol '%s'", item->linkName);
    }
    return new;
}

static int currentOutputLineInSymbolList =0;


static void olcxPrintMenuItemPrefix(FILE *file, BrowserMenu *menu, bool selectable) {
    if (! selectable) {
        fprintf(file, " %s=2", PPCA_SELECTED);
    } else if (menu!=NULL && menu->selected) {
        fprintf(file, " %s=1", PPCA_SELECTED);
    } else {
        fprintf(file, " %s=0", PPCA_SELECTED);
    }

    if (menu != NULL && menu->vlevel==1 && ooBitsGreaterOrEqual(menu->ooBits, OOC_OVERLOADING_APPLICABLE)) {
        fprintf(file, " %s=1", PPCA_BASE);
    } else {
        fprintf(file, " %s=0", PPCA_BASE);
    }

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

static void olcxMenuGenNonVirtualGlobSymList(FILE *file, BrowserMenu *menu) {

    if (currentOutputLineInSymbolList == 1)
        currentOutputLineInSymbolList++; // first line irregularity
    menu->outOnLine = currentOutputLineInSymbolList;
    currentOutputLineInSymbolList++ ;
    ppcIndent();
    fprintf(file,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
    if (menu->referenceable.type!=TypeDefault) {
        fprintf(file," %s=%s", PPCA_TYPE, typeNamesTable[menu->referenceable.type]);
    }
    olcxPrintMenuItemPrefix(file, menu, true);

    char tempString[MAX_CX_SYMBOL_SIZE];
    prettyPrintLinkNameForSymbolInMenu(tempString, menu);
    fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
}

/* Mapped through 'splitMenuPerSymbolsAndMap()' */
static void genNonVirtualsGlobRefLists(BrowserMenu *menu, void *p1) {
    FILE *file = (FILE *)p1;
    BrowserMenu *m;

    // Are there are any visible references at all
    for (m=menu; m!=NULL && !m->visible; m=m->next)
        ;
    if (m == NULL)
        return;

    assert(menu!=NULL);
    ReferenceableItem *r = &menu->referenceable;
    assert(r!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",r->linkName,storagesName[r->storage]);
    for (BrowserMenu *m=menu; m!=NULL; m=m->next) {
        r = &m->referenceable;
        olcxMenuGenNonVirtualGlobSymList(file, m);
    }
}

void splitBrowserMenuAndMap(BrowserMenu *menu, void (*fun)(BrowserMenu *menu, void *p1), void *p1) {
    BrowserMenu    *rr, *mp, **ss, *cc, *all;
    ReferenceableItem *cs;
    all = NULL;
    rr = menu;
    while (rr!=NULL) {
        mp = NULL;
        ss= &rr; cs= &rr->referenceable;
        while (*ss!=NULL) {
            cc = *ss;
            if (isSameReferenceableItem(&cc->referenceable, cs)) {
                // move cc it into map list
                *ss = (*ss)->next;
                cc->next = mp;
                mp = cc;
                goto contlab;
            }
            ss= &(*ss)->next;
        contlab:;
        }
        (*fun)(mp, p1);
        // reconstruct the list in all
        LIST_APPEND(BrowserMenu, mp, all);
        all = mp;
    }
    // now find the original head and make it head,
    // berk, TODO do this by passing pointer to pointer to rrr
    // as parameter
    if (all!=menu) {
        ss = &all;
        while (*ss!=menu && *ss!=NULL)
            ss = &(*ss)->next;
        assert(*ss!=NULL);
        assert (*ss != all);
        *ss = menu->next;
        menu->next = all;
    }
}

static void generateGlobalReferenceLists(BrowserMenu *menu, FILE *file) {
    for (BrowserMenu *m=menu; m!=NULL; m=m->next)
        m->outOnLine = 0;
    currentOutputLineInSymbolList = 1;
    splitBrowserMenuAndMap(menu, genNonVirtualsGlobRefLists, file);
}
