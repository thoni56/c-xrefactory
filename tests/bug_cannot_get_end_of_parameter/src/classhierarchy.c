#include "classhierarchy.h"

#include "menu.h"
#include "misc.h"
#include "cxref.h"
#include "list.h"

#include "protocol.h"


typedef struct integerList {
    int                integer;
    struct integerList *next;
} IntegerList;


static int currentOutputLineInSymbolList =0;


bool isSmallerOrEqClass(int inf, int sup) {
    return false;               /* Dummy replacement */
}

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
    sprintfSymbolLinkName(menu, tempString);
    fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
}

static void genNonVirtualsGlobRefLists(SymbolsMenu *menu, void *p1, char *fn) {
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

void splitMenuPerSymbolsAndMap(SymbolsMenu *menu, void (*fun)(SymbolsMenu *, void *, char *), void *p1,
                               char *p2) {
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
        (*fun)(mp, p1, p2);
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

void generateGlobalReferenceLists(SymbolsMenu *menu, FILE *file, char *fn) {
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next)
        m->outOnLine = 0;
    currentOutputLineInSymbolList = 1;
    splitMenuPerSymbolsAndMap(menu, genNonVirtualsGlobRefLists, file, fn);
}
