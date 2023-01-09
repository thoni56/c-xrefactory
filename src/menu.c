#include "menu.h"

#include "classhierarchy.h"
#include "cxfile.h"
#include "cxref.h"
#include "globals.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "options.h"


void fillSymbolsMenu(SymbolsMenu *menu, ReferencesItem references, bool selected, bool visible,
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
    olcxFree(menu->references.name, strlen(menu->references.name)+1);
    freeReferences(menu->references.references);
    SymbolsMenu *next = menu->next;
    olcxFree(menu, sizeof(*menu));
    return next;
}


void freeSymbolsMenuList(SymbolsMenu *menuList) {
    SymbolsMenu *l;

    l = menuList;
    while (l != NULL) {
        l = freeSymbolsMenu(l);
    }
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

void olcxPrintClassTree(SymbolsMenu *menu) {
    if (options.xref2) {
        ppcBegin(PPC_DISPLAY_CLASS_TREE);
    } else {
        fprintf(communicationChannel, "<");
    }
    scanForClassHierarchy();
    generateGlobalReferenceLists(menu, communicationChannel, "__NO_HTML_FILE_NAME!__");
    if (options.xref2)
        ppcEnd(PPC_DISPLAY_CLASS_TREE);
}
