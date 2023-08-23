#include "classhierarchy.h"

#include "commons.h"
#include "globals.h"
#include "memory.h"
#include "menu.h"
#include "options.h"
#include "misc.h"
#include "cxref.h"
#include "list.h"
#include "filetable.h"

#include "protocol.h"
#include "log.h"


typedef struct integerList {
    int                integer;
    struct integerList *next;
} IntegerList;


static bitArray tmpChRelevant[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChProcessed[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChMarkProcessed[BIT_ARR_DIM(MAX_FILES)];

static SymbolsMenu *tmpVApplClassBackPointersToMenu[MAX_FILES];

static int currentOutputLineInSymbolList =0;


static void clearTmpChRelevant(void) {
    memset(tmpChRelevant, 0, sizeof(tmpChRelevant));
}
static void clearTmpChProcessed(void) {
    memset(tmpChProcessed, 0, sizeof(tmpChProcessed));
}
static void clearTmpChMarkProcessed(void) {
    memset(tmpChMarkProcessed, 0, sizeof(tmpChMarkProcessed));
}
static void clearTmpClassBackPointersToMenu(void) {
    // this should be rather cycle affecting NULLs
    memset(tmpVApplClassBackPointersToMenu, 0, sizeof(tmpVApplClassBackPointersToMenu));
}


ClassHierarchyReference *newClassHierarchyReference(int originFileNumber, int superClass,
                                                    ClassHierarchyReference *next) {
    ClassHierarchyReference *p;

    p = cxAlloc(sizeof(ClassHierarchyReference));
    p->ofile = originFileNumber;
    p->superClass = superClass;
    p->next = next;

    return p;
}


static int isSmallerOrEqClassR(int inferior, int superior, int level) {
    assert(level>0);
    if (inferior == superior)
        return level;

    FileItem *fileItem = getFileItem(inferior);
    for (ClassHierarchyReference *s=fileItem->superClasses; s!=NULL; s=s->next) {
        if (s->superClass == superior) {
            return level+1;
        }
    }
    for (ClassHierarchyReference *s=fileItem->superClasses; s!=NULL; s=s->next) {
        int smallerLevel = isSmallerOrEqClassR(s->superClass, superior, level+1);
        if (smallerLevel) {
            return smallerLevel;
        }
    }
    return 0;
}

bool isSmallerOrEqClass(int inf, int sup) {
    return isSmallerOrEqClassR(inf, sup, 1) != 0;
}

int classCmp(int class1, int class2) {
    int res;

    log_trace("classCMP %s <-> %s", getFileItem(class1)->name, getFileItem(class2)->name);
    res = isSmallerOrEqClassR(class2, class1, 1);
    if (res == 0) {
        res = -isSmallerOrEqClassR(class1, class2, 1);
    }
    return res;
}

bool classHierarchyClassNameLess(int classFileNumber1, int classFileNumber2) {
    char *name;
    int comparison;
    char name1[MAX_FILE_NAME_SIZE];

    FileItem *fileItem1 = getFileItem(classFileNumber1);
    FileItem *fileItem2 = getFileItem(classFileNumber2);

    // SMART, put interface as largest, so they will be at the end
    if (fileItem2->isInterface && ! fileItem1->isInterface)
        return true;
    if (fileItem1->isInterface && ! fileItem2->isInterface)
        return false;
    name = fileItem1->name;
    name = javaGetNudePreTypeName_static(getRealFileName_static(name), options.displayNestedClasses);
    strcpy(name1, name);
    name = fileItem2->name;
    name = javaGetNudePreTypeName_static(getRealFileName_static(name), options.displayNestedClasses);
    comparison = strcmp(name1, name);
    if (comparison!=0)
        return comparison<0;
    comparison = strcmp(fileItem1->name, fileItem2->name);

    return comparison<0;
}

static int classHierarchySuperClassNameLess(ClassHierarchyReference *c1, ClassHierarchyReference *c2) {
    return classHierarchyClassNameLess(c1->superClass, c2->superClass);
}

static int markTransitiveRelevantSubsRec(int fileNumber, int passNumber) {
    FileItem *fileItem = getFileItem(fileNumber);
    if (THEBIT(tmpChMarkProcessed,fileNumber))
        return THEBIT(tmpChRelevant,fileNumber);
    SETBIT(tmpChMarkProcessed, fileNumber);
    for (ClassHierarchyReference *s = fileItem->inferiorClasses; s!=NULL; s=s->next) {
        // do not descend from class to an
        // interface, because of Object -> interface lapsus ?
        // if ((! fi->b.isInterface) && tt->b.isInterface ) continue;
        // do not mix interfaces in first pass
        //&     if (passNumber==FIRST_PASS && temp->b.isInterface) continue;
        if (markTransitiveRelevantSubsRec(s->superClass, passNumber)) {
            //&fprintf(dumpOut,"setting %s relevant\n",fileItem->name);
            SETBIT(tmpChRelevant, fileNumber);
        }
    }
    return THEBIT(tmpChRelevant, fileNumber);
}

static void markTransitiveRelevantSubs(int cind, int passNumber) {
    log_trace("PRE checking %s relevant", getFileItem(cind)->name);
    if (THEBIT(tmpChRelevant,cind)==0) return;
    markTransitiveRelevantSubsRec(cind, passNumber);
}

static void initClassHierarchyGeneration(void) {
    clearTmpChRelevant();
    clearTmpChProcessed();
}

static void setTmpClassBackPointersToMenu(SymbolsMenu *menu) {
    SymbolsMenu *ss;
    clearTmpClassBackPointersToMenu();
    for(ss=menu; ss!=NULL; ss=ss->next) {
        tmpVApplClassBackPointersToMenu[ss->references.vApplClass] = ss;
    }
}

static void genClassHierarchyVerticalBars(FILE *file, IntegerList *nextbars) {
    if (options.xref2) {
        fprintf(file," %s=\"", PPCA_TREE_DEPS);
    }
    if (nextbars!=NULL) {
        LIST_REVERSE(IntegerList, nextbars);
        for (IntegerList *n = nextbars; n!=NULL; n=n->next) {
            if (n->next==NULL) {
                fprintf(file,"  +- ");
            }
            else if (n->integer) fprintf(file,"  | ");
            else fprintf(file,"    ");
        }
        LIST_REVERSE(IntegerList, nextbars);
    }
    if (options.xref2) {
        fprintf(file,"\"");
    }
}


static SymbolsMenu *itemInOriginalList(int fileNumber) {
    if (fileNumber == -1)
        return NULL;
    assert(fileNumber>=0 && fileNumber<MAX_FILES);
    return tmpVApplClassBackPointersToMenu[fileNumber];
}

static void olcxPrintMenuItemPrefix(FILE *file, SymbolsMenu *menu, int selectable) {
    if (options.serverOperation==OLO_CLASS_TREE || options.serverOperation==OLO_SHOW_CLASS_TREE) {
        ; // fprintf(file,"");
    } else if (! selectable) {
        if (options.xref2)
            fprintf(file, " %s=2", PPCA_SELECTED);
        else
            fprintf(file,"- ");
    } else if (menu!=NULL && menu->selected) {
        if (options.xref2)
            fprintf(file, " %s=1", PPCA_SELECTED);
        else
            fprintf(file,"+ ");
    } else {
        if (options.xref2)
            fprintf(file, " %s=0", PPCA_SELECTED);
        else
            fprintf(file,"  ");
    }
    if (menu != NULL && menu->vlevel==1 && ooBitsGreaterOrEqual(menu->ooBits, OOC_PROFILE_APPLICABLE)) {
        if (options.xref2)
            fprintf(file, " %s=1", PPCA_BASE);
        else
            fprintf(file,">>");
    } else {
        if (options.xref2)
            fprintf(file, " %s=0", PPCA_BASE);
        else
            fprintf(file,"  ");
    }
    if (!options.xref2)
        fprintf(file," ");
    if (options.serverOperation==OLO_CLASS_TREE || options.serverOperation==OLO_SHOW_CLASS_TREE) {
        ; //fprintf(ff, "");
    } else if (menu==NULL || (menu->defRefn==0 && menu->refn==0) || !selectable) {
        if (options.xref2)
            fprintf(file, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
        else
            fprintf(file, "  -/-  ");
    } else if (menu->defRefn==0) {
        if (options.xref2)
            fprintf(file, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, menu->refn);
        else
            fprintf(file, "  -/%-3d", menu->refn);
    } else if (menu->refn==0) {
        if (options.xref2)
            fprintf(file, " %s=%d %s=0", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN);
        else
            fprintf(file, "%3d/-  ", menu->defRefn);
    } else {
        if (options.xref2)
            fprintf(file, " %s=%d %s=%d", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN, menu->refn);
        else
            fprintf(file, "%3d/%-3d", menu->defRefn, menu->refn);
    }
    if (!options.xref2)
        fprintf(file,"    ");
}

static void olcxMenuGenNonVirtualGlobSymList(FILE *file, SymbolsMenu *menu) {

    if (currentOutputLineInSymbolList == 1)
        currentOutputLineInSymbolList++; // first line irregularity
    menu->outOnLine = currentOutputLineInSymbolList;
    currentOutputLineInSymbolList++ ;
    if (options.xref2) {
        ppcIndent();
        fprintf(file,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
        if (menu->references.type!=TypeDefault) {
            fprintf(file," %s=%s", PPCA_TYPE, typeNamesTable[menu->references.type]);
        }
        olcxPrintMenuItemPrefix(file, menu, 1);

        char tempString[MAX_CX_SYMBOL_SIZE];
        sprintfSymbolLinkName(menu, tempString);
        fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
    } else {
        fprintf(file,"\n");
        olcxPrintMenuItemPrefix(file, menu, 1);
        printSymbolLinkName(menu, file);
        if (menu->references.type != TypeDefault) {
            fprintf(file,"\t(%s)", typeNamesTable[menu->references.type]);
        }
        //&fprintf(file," ==%s %o (%s) at %x", menu->references.linkName, menu->ooBits, refCategoriesName[menu->references.category], menu);
    }
}

static void printClassHierarchyLineForMenu(SymbolsMenu *menu, FILE *file, int fileNumber,
                                           IntegerList *nextbars) {
    bool alreadyProcessed = THEBIT(tmpChProcessed, fileNumber);
    if (options.xref2) {
        ppcIndent();
        fprintf(file, "<%s %s=%d", PPC_CLASS, PPCA_LINE,
                menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
    }
    olcxPrintMenuItemPrefix(file, menu, !alreadyProcessed);
    if (options.xref2) {
        int indent;
        LIST_LEN(indent, IntegerList, nextbars);
        fprintf(file, " %s=%d", PPCA_INDENT, indent);
    }
    genClassHierarchyVerticalBars(file, nextbars);

    FileItem *fileItem = getFileItem(fileNumber);
    if (menu != NULL) {
        if (menu->references.vApplClass == menu->references.vFunClass && options.serverOperation!=OLO_CLASS_TREE) {
            if (options.xref2)
                fprintf(file, " %s=1", PPCA_DEFINITION);
            else
                fprintf(file,"*");
        }
        menu->visible = true;
    }
    if (fileItem->isInterface) {
        if (options.xref2)
            fprintf(file, " %s=1", PPCA_INTERFACE);
        else
            fprintf(file,"~");
    }
    char *typeName = javaGetNudePreTypeName_static(getRealFileName_static(fileItem->name),
                                                   options.displayNestedClasses);
    if (options.xref2) {
        if (THEBIT(tmpChProcessed,fileNumber))
            fprintf(file," %s=1", PPCA_TREE_UP);
        fprintf(file, " %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(typeName), typeName, PPC_CLASS);
    } else {
        if (THEBIT(tmpChProcessed,fileNumber))
            fprintf(file,"(%s) -> up", typeName);
        else
            fprintf(file,"%s", typeName);
        fprintf(file,"\n");
    }
}


static void descendTheClassHierarchy(SymbolsMenu *menu, FILE *file,
                                     int vApplCl, int oldvFunCl,
                                     int level,
                                     IntegerList *nextbars,
                                     int passNumber
) {
    IntegerList snextbar;
    ClassHierarchyReference *snext;
    SymbolsMenu *itt;
    int vFunCl;

    FileItem *fileItem = getFileItem(vApplCl);
    if (THEBIT(tmpChRelevant,vApplCl)==0) return;
    itt = itemInOriginalList(vApplCl);

    if (itt == NULL) {
        assert(menu);
        vFunCl = oldvFunCl;
        // O.K. create new item, so that browse class action will work
        itt = olCreateNewMenuItem(&menu->references, vApplCl, vFunCl, &noPosition, UsageNone,
                                  0, 1, 0, UsageNone, 0);
        // insert it into the list, no matter where?
        itt->next = menu->next;
        menu->next = itt;
        tmpVApplClassBackPointersToMenu[vApplCl] = itt;
    } else {
        vFunCl = itt->references.vFunClass;
    }

    if (currentOutputLineInSymbolList == 1)
        currentOutputLineInSymbolList++; // first line irregularity
    if (itt!=NULL && itt->outOnLine==0)
        itt->outOnLine = currentOutputLineInSymbolList;
    currentOutputLineInSymbolList ++;
    printClassHierarchyLineForMenu(itt, file, vApplCl, nextbars);

    if (THEBIT(tmpChProcessed,vApplCl)==1)
        return;
    SETBIT(tmpChProcessed, vApplCl);

    // putting the following in comments makes that for -refnum==1
    // subclasses will not be sorted !
    // also subclasses for on-line resolution would not be sorted!
    LIST_MERGE_SORT(ClassHierarchyReference, fileItem->inferiorClasses, classHierarchySuperClassNameLess);

    ClassHierarchyReference *s=fileItem->inferiorClasses;
    while (s!=NULL) {
        assert(getFileItem(s->superClass));
        snext = s->next;
        while (snext!=NULL && THEBIT(tmpChRelevant,snext->superClass)==0) {
            snext = snext->next;
        }
        snextbar = (IntegerList) {.integer = (snext!=NULL), .next = nextbars};
        descendTheClassHierarchy(menu, file, s->superClass, vFunCl, level+1,
                                 &snextbar, passNumber);
        s = snext;
    }
}

static bool genThisClassHierarchy(SymbolsMenu *menu, int vApplCl, int oldvFunCl, FILE *file,
                                  int passNumber) {
    FileItem *fileItem = getFileItem(vApplCl);
    if (fileItem==NULL)
        return false;
    if (THEBIT(tmpChProcessed, vApplCl))
        return false;
    if (THEBIT(tmpChRelevant, vApplCl)==0)
        return false;
    // check if you are at the top of a sub-hierarchy
    for (ClassHierarchyReference *s = fileItem->superClasses; s!=NULL; s=s->next) {
        if (THEBIT(tmpChRelevant,s->superClass) && THEBIT(tmpChProcessed,s->superClass)==0)
            return false;
    }
    // yes I am on the top, recursively descent and print all subclasses
    if (passNumber==FIRST_PASS && fileItem->isInterface)
        return false;
    descendTheClassHierarchy(menu, file, vApplCl, oldvFunCl, 0, NULL, passNumber);
    return true;
}

void genClassHierarchies(SymbolsMenu *menuList, FILE *file, int passNumber) {
    // mark the classes where the method is defined and used
    clearTmpChRelevant();
    for (SymbolsMenu *menu=menuList; menu!=NULL; menu=menu->next) {
        assert(getFileItem(menu->references.vApplClass));
        if (menu->visible) {
            SETBIT(tmpChRelevant, menu->references.vApplClass);
            SETBIT(tmpChRelevant, menu->references.vFunClass);
        }
    }
    // now, mark the relevant subtree of class tree
    clearTmpChMarkProcessed();
    for (SymbolsMenu *menu=menuList; menu!=NULL; menu=menu->next) {
        markTransitiveRelevantSubs(menu->references.vFunClass, passNumber);
        markTransitiveRelevantSubs(menu->references.vApplClass, passNumber);
    }
    // and gen the class subhierarchy
    for (SymbolsMenu *menu=menuList; menu!=NULL; menu=menu->next) {
        genThisClassHierarchy(menuList, menu->references.vFunClass, NO_FILE_NUMBER, file, passNumber);
        genThisClassHierarchy(menuList, menu->references.vApplClass, NO_FILE_NUMBER, file, passNumber);
    }
}

static void olcxMenuGenGlobRefsForVirtMethod(SymbolsMenu *menu, FILE *file) {
    char linkName[MAX_REF_LEN];

    linkNamePrettyPrint(linkName, menu->references.linkName, MAX_REF_LEN, SHORT_NAME);
    if (strcmp(menu->references.linkName, LINK_NAME_CLASS_TREE_ITEM)!=0) {
        if (options.xref2)
            ppcGenRecord(PPC_VIRTUAL_SYMBOL, linkName);
        else
            fprintf(file, "\n== %s\n", linkName);
        currentOutputLineInSymbolList += 2 ;
    }
    initClassHierarchyGeneration();
    setTmpClassBackPointersToMenu(menu);
    genClassHierarchies(menu, file, FIRST_PASS);
    setTmpClassBackPointersToMenu(menu);
    genClassHierarchies(menu, file, SECOND_PASS);
}

static int isVirtualMenuItem(ReferencesItem *r) {
    return (r->storage == StorageField
            || r->storage == StorageMethod
            || r->storage == StorageConstructor);
}

static void genVirtualsGlobRefLists(SymbolsMenu *menu, void *p1, char *fn) {
    FILE *file = (FILE *)p1;
    SymbolsMenu    *s;
    ReferencesItem *r;

    // first count if there are some references at all
    for (s = menu; s != NULL && !s->visible; s = s->next)
        ;
    if (s == NULL)
        return;
    assert(menu != NULL);
    r = &menu->references;
    assert(r != NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",r->linkName,storagesName[r->storage]);
    if (isVirtualMenuItem(r)) {
        olcxMenuGenGlobRefsForVirtMethod(menu, file);
    }
}

static void genNonVirtualsGlobRefLists(SymbolsMenu *menu, void *p1, char *fn) {
    FILE *file = (FILE *)p1;
    SymbolsMenu    *m;
    ReferencesItem *r;

    // Are there are any visible references at all
    for (m=menu; m!=NULL && !m->visible; m=m->next)
        ;
    if (m == NULL)
        return;

    assert(menu!=NULL);
    r = &menu->references;
    assert(r!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",r->linkName,storagesName[r->storage]);
    if (! isVirtualMenuItem(r)) {
        for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
            r = &m->references;
            olcxMenuGenNonVirtualGlobSymList(file, m);
        }
    }
}

void splitMenuPerSymbolsAndMap(SymbolsMenu *menu, void (*fun)(SymbolsMenu *, void *, char *), void *p1,
                               char *p2) {
    SymbolsMenu    *rr, *mp, **ss, *cc, *all;
    ReferencesItem *cs;
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
    splitMenuPerSymbolsAndMap(menu, genVirtualsGlobRefLists, file, fn);
}
