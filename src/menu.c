#include "menu.h"

#include "cxref.h"
#include "globals.h"
#include "filetable.h"
#include "list.h"
#include "log.h"

void fillSymbolsMenu(SymbolsMenu *symbolsMenu, ReferencesItem references, bool selected, bool visible,
                     unsigned ooBits, char olUsage, short int vlevel, char defUsage, Position defpos) {
    symbolsMenu->references = references;
    symbolsMenu->selected   = selected;
    symbolsMenu->visible    = visible;
    symbolsMenu->ooBits     = ooBits;
    symbolsMenu->olUsage    = olUsage;
    symbolsMenu->vlevel     = vlevel;
    symbolsMenu->defUsage   = defUsage;
    symbolsMenu->defpos     = defpos;

    /* Default values */
    symbolsMenu->refn      = 0;
    symbolsMenu->defRefn   = 0;
    symbolsMenu->outOnLine = 0;
    symbolsMenu->markers   = NULL;
    symbolsMenu->next      = NULL;
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
