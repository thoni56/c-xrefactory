#include "menu.h"

#include "classhierarchy.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "ppc.h"
#include "reference.h"


void fillSymbolsMenu(SymbolsMenu *menu, ReferenceItem references, bool selected, bool visible,
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

SymbolsMenu *freeSymbolsMenu(SymbolsMenu *menu) {
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
    added = olcxAddReference(&menu->references.references, reference);
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

SymbolsMenu *olCreateNewMenuItem(ReferenceItem *symbol, int vApplClass, int vFunCl, Position *defpos,
                                 int defusage, int selected, int visible, unsigned ooBits, int olusage,
                                 int vlevel) {
    SymbolsMenu   *symbolsMenu;
    ReferenceItem refItem;
    char          *allocatedNameCopy;

    allocatedNameCopy = olcxStringCopy(symbol->linkName);

    fillReferenceItem(&refItem, allocatedNameCopy, vApplClass,
                       symbol->type, symbol->storage, symbol->scope,
                       symbol->visibility);

    symbolsMenu = malloc(sizeof(SymbolsMenu));
    fillSymbolsMenu(symbolsMenu, refItem, selected, visible, ooBits, olusage, vlevel, defusage, *defpos);
    return symbolsMenu;
}

static bool referenceItemIsLess(ReferenceItem *s1, ReferenceItem *s2) {
    int cmp;

    cmp = strcmp(s1->linkName, s2->linkName);
    if (cmp < 0)
        return true;
    else if (cmp > 0)
        return false;
    if (s1->vApplClass < s2->vApplClass)
        return true;
    else if (s1->vApplClass > s2->vApplClass)
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

SymbolsMenu *olAddBrowsedSymbolToMenu(SymbolsMenu **menuP, ReferenceItem *symbol,
                                      bool selected, bool visible, unsigned ooBits,
                                      int olusage, int vlevel,
                                      Position *defpos, int defusage) {
    SymbolsMenu *new, **place, dummyMenu;

    fillSymbolsMenu(&dummyMenu, *symbol, 0, false, 0, olusage, vlevel, UsageNone, noPosition);
    SORTED_LIST_PLACE3(place, SymbolsMenu, &dummyMenu, menuP, olSymbolMenuIsLess);
    new = *place;
    if (*place==NULL || olSymbolMenuIsLess(&dummyMenu, *place)) {
        assert(symbol);
        new = olCreateNewMenuItem(symbol, symbol->vApplClass, symbol->vApplClass, defpos, defusage,
                                selected, visible, ooBits,
                                olusage, vlevel);
        LIST_CONS(new, *place);
        log_trace(":adding browsed symbol '%s'", symbol->linkName);
    }
    return new;
}
