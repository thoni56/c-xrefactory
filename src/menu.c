#include "menu.h"

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
#include "reference.h"


static void fillSymbolsMenu(SymbolsMenu *menu, ReferenceItem references, bool selected, bool visible,
                     unsigned ooBits, char olUsage, short int vlevel, char defUsage, Position defpos) {
    menu->references = references;
    menu->selected   = selected;
    menu->visible    = visible;
    menu->ooBits     = ooBits;
    menu->olUsage    = olUsage;
    menu->vlevel     = vlevel;
    menu->defUsage   = defUsage;
    menu->defpos     = defpos;

    /* Default values */
    menu->refn      = 0;
    menu->defRefn   = 0;
    menu->outOnLine = 0;
    menu->markers   = NULL;
    menu->next      = NULL;
}

SymbolsMenu makeSymbolsMenu(ReferenceItem references, bool selected, bool visible,
                            unsigned ooBits, char olUsage, short int vlevel, char defUsage, Position defpos) {
    SymbolsMenu menu;
    fillSymbolsMenu(&menu, references, selected, visible, ooBits, olUsage, vlevel, defUsage, defpos);
    return menu;
}

static SymbolsMenu *freeSymbolsMenu(SymbolsMenu *menu) {
    free(menu->references.linkName);
    freeReferences(menu->references.references);
    SymbolsMenu *next = menu->next;
    free(menu);
    return next;
}


void freeSymbolsMenuList(SymbolsMenu *menuList) {
    SymbolsMenu *l;

    l = menuList;
    while (l != NULL) {
        l = freeSymbolsMenu(l);
    }
}

void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference) {
    Reference *added;
    added = addReferenceToList(&menu->references.references, reference);
    if (reference->usage == UsageClassTreeDefinition) menu->defpos = reference->position;
    if (added!=NULL) {
        if (isDefinitionOrDeclarationUsage(reference->usage)) {
            if (reference->usage==UsageDefined && positionsAreEqual(reference->position, menu->defpos)) {
                added->usage = UsageOLBestFitDefined;
            }
            if (reference->usage < menu->defUsage) {
                menu->defUsage = reference->usage;
                menu->defpos = reference->position;
            }
            menu->defRefn ++;
        } else {
            menu->refn ++;
        }
    }
}

static void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file);

void olcxPrintSelectionMenu(SymbolsMenu *menu) {
    assert(options.xref2);

    ppcBegin(PPC_SYMBOL_RESOLUTION);
    if (menu!=NULL) {
        generateGlobalReferenceLists(menu, communicationChannel);
    }
    ppcEnd(PPC_SYMBOL_RESOLUTION);
}

static char *olcxStringCopy(char *string) {
    char *copy = strdup(string);
    return copy;
}

SymbolsMenu *createNewMenuItem(ReferenceItem *symbol, int includedFileNumber, Position defpos,
                               int defusage, int selected, int visible, unsigned ooBits, Usage olusage,
                               int vlevel) {
    SymbolsMenu   *symbolsMenu;
    char          *allocatedNameCopy;

    allocatedNameCopy = olcxStringCopy(symbol->linkName);

    ReferenceItem refItem = makeReferenceItem(allocatedNameCopy, symbol->type, symbol->storage, symbol->scope,
                                              symbol->visibility, includedFileNumber);

    symbolsMenu = malloc(sizeof(SymbolsMenu));
    fillSymbolsMenu(symbolsMenu, refItem, selected, visible, ooBits, olusage, vlevel, defusage, defpos);
    return symbolsMenu;
}

static bool referenceItemIsLess(ReferenceItem *s1, ReferenceItem *s2) {
    int cmp;

    cmp = strcmp(s1->linkName, s2->linkName);
    if (cmp < 0)
        return true;
    else if (cmp > 0)
        return false;
    if (s1->includedFileNumber < s2->includedFileNumber)
        return true;
    else if (s1->includedFileNumber > s2->includedFileNumber)
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

static bool olSymbolMenuIsLess(SymbolsMenu *s1, SymbolsMenu *s2) {
    return referenceItemIsLess(&s1->references, &s2->references);
}

SymbolsMenu *addBrowsedSymbolToMenu(SymbolsMenu **menuP, ReferenceItem *symbol,
                                    bool selected, bool visible, unsigned ooBits,
                                    int olusage, int vlevel,
                                    Position defpos, int defusage) {
    SymbolsMenu *new, **place, dummyMenu;

    fillSymbolsMenu(&dummyMenu, *symbol, 0, false, 0, olusage, vlevel, UsageNone, noPosition);
    SORTED_LIST_PLACE3(place, SymbolsMenu, &dummyMenu, menuP, olSymbolMenuIsLess);
    new = *place;
    if (*place==NULL || olSymbolMenuIsLess(&dummyMenu, *place)) {
        assert(symbol);
        new = createNewMenuItem(symbol, symbol->includedFileNumber, defpos, defusage,
                                selected, visible, ooBits,
                                olusage, vlevel);
        LIST_CONS(new, *place);
        log_trace(":adding browsed symbol '%s'", symbol->linkName);
    }
    return new;
}

static int currentOutputLineInSymbolList =0;


static void olcxPrintMenuItemPrefix(FILE *file, SymbolsMenu *menu, bool selectable) {
    if (! selectable) {
        fprintf(file, " %s=2", PPCA_SELECTED);
    } else if (menu!=NULL && menu->selected) {
        fprintf(file, " %s=1", PPCA_SELECTED);
    } else {
        fprintf(file, " %s=0", PPCA_SELECTED);
    }

    if (menu != NULL && menu->vlevel==1 && ooBitsGreaterOrEqual(menu->ooBits, OOC_PROFILE_APPLICABLE)) {
        fprintf(file, " %s=1", PPCA_BASE);
    } else {
        fprintf(file, " %s=0", PPCA_BASE);
    }

    if (menu==NULL || (menu->defRefn==0 && menu->refn==0) || !selectable) {
        fprintf(file, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
    } else if (menu->defRefn==0) {
        fprintf(file, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, menu->refn);
    } else if (menu->refn==0) {
        fprintf(file, " %s=%d %s=0", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN);
    } else {
        fprintf(file, " %s=%d %s=%d", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN, menu->refn);
    }
}

static void olcxMenuGenNonVirtualGlobSymList(FILE *file, SymbolsMenu *menu) {

    if (currentOutputLineInSymbolList == 1)
        currentOutputLineInSymbolList++; // first line irregularity
    menu->outOnLine = currentOutputLineInSymbolList;
    currentOutputLineInSymbolList++ ;
    ppcIndent();
    fprintf(file,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
    if (menu->references.type!=TypeDefault) {
        fprintf(file," %s=%s", PPCA_TYPE, typeNamesTable[menu->references.type]);
    }
    olcxPrintMenuItemPrefix(file, menu, true);

    char tempString[MAX_CX_SYMBOL_SIZE];
    prettyPrintLinkNameForSymbolInMenu(tempString, menu);
    fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
}

/* Mapped through 'splitMenuPerSymbolsAndMap()' */
static void genNonVirtualsGlobRefLists(SymbolsMenu *menu, void *p1) {
    FILE *file = (FILE *)p1;
    SymbolsMenu    *m;
    ReferenceItem *r;

    // Are there are any visible references at all
    for (m=menu; m!=NULL && !m->visible; m=m->next)
        ;
    if (m == NULL)
        return;

    assert(menu!=NULL);
    r = &menu->references;
    assert(r!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",r->linkName,storagesName[r->storage]);
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        r = &m->references;
        olcxMenuGenNonVirtualGlobSymList(file, m);
    }
}

void splitMenuPerSymbolsAndMap(SymbolsMenu *menu, void (*fun)(SymbolsMenu *menu, void *p1), void *p1) {
    SymbolsMenu    *rr, *mp, **ss, *cc, *all;
    ReferenceItem *cs;
    all = NULL;
    rr = menu;
    while (rr!=NULL) {
        mp = NULL;
        ss= &rr; cs= &rr->references;
        while (*ss!=NULL) {
            cc = *ss;
            if (isSameCxSymbol(&cc->references, cs)) {
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
        LIST_APPEND(SymbolsMenu, mp, all);
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

static void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file) {
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next)
        m->outOnLine = 0;
    currentOutputLineInSymbolList = 1;
    splitMenuPerSymbolsAndMap(menu, genNonVirtualsGlobRefLists, file);
}
