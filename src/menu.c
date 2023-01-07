#include "menu.h"

#include "cxref.h"
#include "globals.h"
#include "filetable.h"
#include "list.h"
#include "log.h"


void fillSymbolsMenu(SymbolsMenu *symbolsMenu,
                     ReferencesItem references,
                     bool selected,
                     bool visible,
                     unsigned ooBits,
                     char olUsage,
                     short int vlevel,
                     short int refn,
                     short int defRefn,
                     char defUsage,
                     Position defpos,
                     int outOnLine,
                     EditorMarkerList *markers,	/* for refactory only */
                     SymbolsMenu *next
) {
    symbolsMenu->references = references;
    symbolsMenu->selected = selected;
    symbolsMenu->visible = visible;
    symbolsMenu->ooBits = ooBits;
    symbolsMenu->olUsage = olUsage;
    symbolsMenu->vlevel = vlevel;
    symbolsMenu->refn = refn;
    symbolsMenu->defRefn = defRefn;
    symbolsMenu->defUsage = defUsage;
    symbolsMenu->defpos = defpos;
    symbolsMenu->outOnLine = outOnLine;
    symbolsMenu->markers = markers;
    symbolsMenu->next= next;
}


void olcxAddReferenceToSymbolsMenu(SymbolsMenu *menu, Reference *reference, int bestFitFlag) {
    Reference *added;
    added = olcxAddReference(&menu->references.references, reference, bestFitFlag);
    if (reference->usage.kind == UsageClassTreeDefinition) menu->defpos = reference->position;
    if (added!=NULL) {
        if (isDefinitionOrDeclarationUsage(reference->usage.kind)) {
            if (reference->usage.kind==UsageDefined && positionsAreEqual(reference->position, menu->defpos)) {
                added->usage.kind = UsageOLBestFitDefined;
            }
            if (reference->usage.kind < menu->defUsage) {
                menu->defUsage = reference->usage.kind;
                menu->defpos = reference->position;
            }
            menu->defRefn ++;
        } else {
            menu->refn ++;
        }
    }
}

// Checking if this can be moved here...
#if 0
void olCreateSelectionMenu(int command) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;
    int                 fnum;

    // I think this ordering is useless
    LIST_MERGE_SORT(SymbolsMenu,
                    sessionData.browserStack.top->hkSelectedSym,
                    refItemsOrderLess);
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    if (ss == NULL) return;
    renameCollationSymbols(ss);
    LIST_SORT(SymbolsMenu, rstack->hkSelectedSym, olMenuHashFileNumLess);
    ss = rstack->hkSelectedSym;
    while (ss!=NULL) {
        scanReferencesToCreateMenu(ss->references.name);
        fnum = cxFileHashNumber(ss->references.name);
        while (ss!=NULL && fnum==cxFileHashNumber(ss->references.name))
            ss = ss->next;
    }
    mapOverReferenceTable(mapCreateSelectionMenu);
    mapOverReferenceTable(putOnLineLoadedReferences);
    setSelectedVisibleItems(rstack->menuSym, command, rstack->menuFilterLevel);
    assert(rstack->references==NULL);
    olProcessSelectedReferences(rstack, genOnLineReferences);
    // isn't ordering useless ?
    LIST_MERGE_SORT(SymbolsMenu,
                    sessionData.browserStack.top->menuSym,
                    refItemsOrderLess);
}

#endif
