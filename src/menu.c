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

void olcxPrintSelectionMenu(SymbolsMenu *menu) {
    if (options.xref2) {
        ppcBegin(PPC_SYMBOL_RESOLUTION);
    } else {
        fprintf(communicationChannel, ">");
    }
    if (menu!=NULL) {
        scanForClassHierarchy();
        generateGlobalReferenceLists(menu, communicationChannel, "__NO_HTML_FILE_NAME!__");
    }
    if (options.xref2) {
        ppcEnd(PPC_SYMBOL_RESOLUTION);
    } else {
        if (options.serverOperation==OLO_RENAME || options.serverOperation==OLO_ARG_MANIP || options.serverOperation==OLO_ENCAPSULATE) {
            if (LANGUAGE(LANG_JAVA)) {
                fprintf(communicationChannel, "-![Warning] It is highly recommended to process the whole hierarchy of related classes at once. Unselection of any class of applications above (and its exclusion from refactoring process) may cause changes in your program behavior. Press <return> to continue.\n");
            } else {
                fprintf(communicationChannel, "-![Warning] It is highly recommended to process all symbols at once. Unselection of any symbols and its exclusion from refactoring process may cause changes in your program behavior. Press <return> to continue.\n");
            }
        }
        if (options.serverOperation==OLO_VIRTUAL2STATIC_PUSH) {
            fprintf(communicationChannel, "-![Warning] If you see this message it is highly probable that turning this virtual method into static will not be behaviour preserving! This refactoring is behaviour preserving only if the method does not use mechanism of virtual invocations. On this screen you should select the application classes which are refering to the method which will become static. If you can't unambiguously determine those references do not continue in this refactoring!\n");
        }
        if (options.serverOperation==OLO_SAFETY_CHECK2) {
            if (LANGUAGE(LANG_JAVA)) {
                fprintf(communicationChannel, "-![Warning] There are differences between original class hierarchy and the new one, those name clashes may cause that the refactoring will not be behavior preserving!\n");
            } else {
                fprintf(communicationChannel, "-![Error] There is differences between original and new symbols referenced at this position. The difference is due to name clashes and may cause changes in the behaviour of the program. Please, undo last refactoring!");
            }
        }
        if (options.serverOperation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK) {
            fprintf(communicationChannel, "-![Warning] A method (getter or setter) created during the encapsulation has the same name as an existing method, so it will be inserted into this (existing) inheritance hierarchy. This may cause that the refactoring will not be behaviour preserving. Please, select applications unambiguously reporting to the newly created method. If you can't do this, you should undo the refactoring and rename the field first!\n");
        }
    }
}
