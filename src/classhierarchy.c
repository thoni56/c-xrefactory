#include "classhierarchy.h"

#include "commons.h"
#include "globals.h"
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

static int s_symbolListOutputCurrentLine =0;


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


ClassHierarchyReference *newClassHierarchyReference(int originFileIndex, int superClass,
                                                    ClassHierarchyReference *next) {
    ClassHierarchyReference *p;

    CX_ALLOC(p, ClassHierarchyReference);
    p->ofile = originFileIndex;
    p->superClass = superClass;
    p->next = next;

    return p;
}


bool classHierarchyClassNameLess(int classFileIndex1, int classFileIndex2) {
    char *name;
    int comparison;
    char name1[MAX_FILE_NAME_SIZE];

    FileItem *fileItem1 = getFileItem(classFileIndex1);
    FileItem *fileItem2 = getFileItem(classFileIndex2);

    // SMART, put interface as largest, so they will be at the end
    if (fileItem2->bits.isInterface && ! fileItem1->bits.isInterface)
        return true;
    if (fileItem1->bits.isInterface && ! fileItem2->bits.isInterface)
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

static int markTransitiveRelevantSubsRec(int fileIndex, int passNumber) {
    FileItem *fileItem = getFileItem(fileIndex);
    if (THEBIT(tmpChMarkProcessed,fileIndex))
        return THEBIT(tmpChRelevant,fileIndex);
    SETBIT(tmpChMarkProcessed, fileIndex);
    for (ClassHierarchyReference *s = fileItem->inferiorClasses; s!=NULL; s=s->next) {
        // do not descend from class to an
        // interface, because of Object -> interface lapsus ?
        // if ((! fi->b.isInterface) && tt->b.isInterface ) continue;
        // do not mix interfaces in first pass
        //&     if (passNumber==FIRST_PASS && temp->b.isInterface) continue;
        if (markTransitiveRelevantSubsRec(s->superClass, passNumber)) {
            //&fprintf(dumpOut,"setting %s relevant\n",fileItem->name);
            SETBIT(tmpChRelevant, fileIndex);
        }
    }
    return THEBIT(tmpChRelevant, fileIndex);
}

static void markTransitiveRelevantSubs(int cind, int passNumber) {
    log_trace("PRE checking %s relevant", getFileItem(cind)->name);
    if (THEBIT(tmpChRelevant,cind)==0) return;
    markTransitiveRelevantSubsRec(cind, passNumber);
}

void classHierarchyGenInit(void) {
    clearTmpChRelevant();
    clearTmpChProcessed();
}

void setTmpClassBackPointersToMenu(SymbolsMenu *menu) {
    SymbolsMenu *ss;
    clearTmpClassBackPointersToMenu();
    for(ss=menu; ss!=NULL; ss=ss->next) {
        tmpVApplClassBackPointersToMenu[ss->s.vApplClass] = ss;
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


static SymbolsMenu *itemInOriginalList(int fileIndex) {
    if (fileIndex == -1)
        return NULL;
    assert(fileIndex>=0 && fileIndex<MAX_FILES);
    return tmpVApplClassBackPointersToMenu[fileIndex];
}

static void olcxPrintMenuItemPrefix(FILE *file, SymbolsMenu *menu, int selectable) {
    if (options.server_operation==OLO_CLASS_TREE || options.server_operation==OLO_SHOW_CLASS_TREE) {
        ; // fprintf(file,"");
    } else if (! selectable) {
        if (options.xref2) fprintf(file, " %s=2", PPCA_SELECTED);
        else fprintf(file,"- ");
    } else if (menu!=NULL && menu->selected) {
        if (options.xref2) fprintf(file, " %s=1", PPCA_SELECTED);
        else fprintf(file,"+ ");
    } else {
        if (options.xref2) fprintf(file, " %s=0", PPCA_SELECTED);
        else fprintf(file,"  ");
    }
    if (menu != NULL && menu->vlevel==1 && ooBitsGreaterOrEqual(menu->ooBits, OOC_PROFILE_APPLICABLE)) {
        if (options.xref2) fprintf(file, " %s=1", PPCA_BASE);
        else fprintf(file,">>");
    } else {
        if (options.xref2) fprintf(file, " %s=0", PPCA_BASE);
        else fprintf(file,"  ");
    }
    if (!options.xref2) fprintf(file," ");
    if (options.server_operation==OLO_CLASS_TREE || options.server_operation==OLO_SHOW_CLASS_TREE) {
        ; //fprintf(ff, "");
    } else if (menu==NULL || (menu->defRefn==0 && menu->refn==0) || !selectable) {
        if (options.xref2) fprintf(file, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
        else fprintf(file, "  -/-  ");
    } else if (menu->defRefn==0) {
        if (options.xref2) fprintf(file, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, menu->refn);
        else fprintf(file, "  -/%-3d", menu->refn);
    } else if (menu->refn==0) {
        if (options.xref2) fprintf(file, " %s=%d %s=0", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN);
        else fprintf(file, "%3d/-  ", menu->defRefn);
    } else {
        if (options.xref2) fprintf(file, " %s=%d %s=%d", PPCA_DEF_REFN, menu->defRefn, PPCA_REFN, menu->refn);
        else fprintf(file, "%3d/%-3d", menu->defRefn, menu->refn);
    }
    if (!options.xref2)
        fprintf(file,"    ");
}

static void olcxMenuGenNonVirtualGlobSymList(FILE *file, SymbolsMenu *menu) {

    if (s_symbolListOutputCurrentLine == 1)
        s_symbolListOutputCurrentLine++; // first line irregularity
    menu->outOnLine = s_symbolListOutputCurrentLine;
    s_symbolListOutputCurrentLine++ ;
    if (options.xref2) {
        ppcIndent();
        fprintf(file,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, menu->outOnLine+SYMBOL_MENU_FIRST_LINE);
        if (menu->s.bits.symType!=TypeDefault) {
            fprintf(file," %s=%s", PPCA_TYPE, typeNamesTable[menu->s.bits.symType]);
        }
        olcxPrintMenuItemPrefix(file, menu, 1);

        char tempString[MAX_CX_SYMBOL_SIZE];
        sprintfSymbolLinkName(tempString, menu);
        fprintf(file," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(tempString), tempString, PPC_SYMBOL);
    } else {
        fprintf(file,"\n");
        olcxPrintMenuItemPrefix(file, menu, 1);
        printSymbolLinkName(file, menu);
        if (menu->s.bits.symType != TypeDefault) {
            fprintf(file,"\t(%s)", typeNamesTable[menu->s.bits.symType]);
        }
        //&fprintf(file," ==%s %o (%s) at %x", menu->s.name, menu->ooBits, refCategoriesName[menu->s.bits.category], menu);
    }
}

static void olcxMenuPrintClassHierarchyLine(FILE *file, int fileIndex,
                                            IntegerList *nextbars,
                                            SymbolsMenu *menu) {
    bool alreadyProcessed = THEBIT(tmpChProcessed, fileIndex);
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

    FileItem *fileItem = getFileItem(fileIndex);
    if (menu != NULL) {
        if (menu->s.vApplClass == menu->s.vFunClass && options.server_operation!=OLO_CLASS_TREE) {
            if (options.xref2)
                fprintf(file, " %s=1", PPCA_DEFINITION);
            else
                fprintf(file,"*");
        }
        menu->visible = 1;
    }
    if (fileItem->bits.isInterface) {
        if (options.xref2)
            fprintf(file, " %s=1", PPCA_INTERFACE);
        else
            fprintf(file,"~");
    }
    char *typeName = javaGetNudePreTypeName_static(getRealFileName_static(fileItem->name),
                                                   options.displayNestedClasses);
    if (options.xref2) {
        if (THEBIT(tmpChProcessed,fileIndex))
            fprintf(file," %s=1", PPCA_TREE_UP);
        fprintf(file, " %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(typeName), typeName, PPC_CLASS);
    } else {
        if (THEBIT(tmpChProcessed,fileIndex))
            fprintf(file,"(%s) -> up", typeName);
        else
            fprintf(file,"%s", typeName);
        fprintf(file,"\n");
    }
}


static void descendTheClassHierarchy(FILE *file,
                                     int vApplCl, int oldvFunCl,
                                     SymbolsMenu *menu,
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
        itt = olCreateNewMenuItem(&menu->s, vApplCl, vFunCl, &noPosition, UsageNone,
                                  0, 1, 0, UsageNone, 0);
        // insert it into the list, no matter where?
        itt->next = menu->next;
        menu->next = itt;
        tmpVApplClassBackPointersToMenu[vApplCl] = itt;
    } else {
        vFunCl = itt->s.vFunClass;
    }

    if (s_symbolListOutputCurrentLine == 1) s_symbolListOutputCurrentLine++; // first line irregularity
    if (itt!=NULL && itt->outOnLine==0) itt->outOnLine = s_symbolListOutputCurrentLine;
    s_symbolListOutputCurrentLine ++;
    olcxMenuPrintClassHierarchyLine(file, vApplCl, nextbars, itt);

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
        descendTheClassHierarchy(file, s->superClass, vFunCl, menu, level+1,
                                 &snextbar, passNumber);
        s = snext;
    }
}

static bool genThisClassHierarchy(int vApplCl, int oldvFunCl,
                                 FILE *file,
                                 SymbolsMenu *menu,
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
    if (passNumber==FIRST_PASS && fileItem->bits.isInterface)
        return false;
    descendTheClassHierarchy(file, vApplCl, oldvFunCl, menu, 0, NULL, passNumber);
    return true;
}

void genClassHierarchies(FILE *file, SymbolsMenu *menuList,
                         int virtualFlag, int passNumber) {
    SymbolsMenu *menu;

    // mark the classes where the method is defined and used
    clearTmpChRelevant();
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        assert(getFileItem(menu->s.vApplClass));
        if (menu->visible) {
            SETBIT(tmpChRelevant, menu->s.vApplClass);
            SETBIT(tmpChRelevant, menu->s.vFunClass);
        }
    }
    // now, mark the relevant subtree of class tree
    clearTmpChMarkProcessed();
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        markTransitiveRelevantSubs(menu->s.vFunClass, passNumber);
        markTransitiveRelevantSubs(menu->s.vApplClass, passNumber);
    }
    // and gen the class subhierarchy
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        genThisClassHierarchy(menu->s.vFunClass, noFileIndex, file, menuList, passNumber);
        genThisClassHierarchy(menu->s.vApplClass, noFileIndex, file, menuList, passNumber);
    }
}

static void olcxMenuGenGlobRefsForVirtMethod(FILE *ff, SymbolsMenu *rrr) {
    char ln[MAX_REF_LEN];

    linkNamePrettyPrint(ln,rrr->s.name,MAX_REF_LEN,SHORT_NAME);
    if (strcmp(rrr->s.name, LINK_NAME_CLASS_TREE_ITEM)==0) {
        /*&
          fprintf(ff, "\n");
          s_symbolListOutputCurrentLine += 1 ;
          &*/
    } else {
        if (options.xref2) ppcGenRecord(PPC_VIRTUAL_SYMBOL, ln);
        else fprintf(ff, "\n== %s\n", ln);
        s_symbolListOutputCurrentLine += 2 ;
    }
    classHierarchyGenInit();
    setTmpClassBackPointersToMenu(rrr);
    genClassHierarchies( ff, rrr, SINGLE_VIRT_ITEM, FIRST_PASS);
    //&fprintf(ff,"interfaces:\n");
    setTmpClassBackPointersToMenu(rrr);
    genClassHierarchies( ff, rrr, SINGLE_VIRT_ITEM, SECOND_PASS);
    //& if (! options.xref2) fprintf(ff, "\n");
    //& s_symbolListOutputCurrentLine ++ ;
}

static int isVirtualMenuItem(ReferencesItem *p) {
    return (p->bits.storage == StorageField
            || p->bits.storage == StorageMethod
            || p->bits.storage == StorageConstructor);
}

static void genVirtualsGlobRefLists(    SymbolsMenu *rrr,
                                        FILE *ff,
                                        char *fn
                                        ) {
    SymbolsMenu *ss;
    ReferencesItem *p;

    // first count if there are some references at all
    for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
    if (ss == NULL) return;
    assert(rrr!=NULL);
    p = &rrr->s;
    assert(p!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->bits.storage]);
    if (isVirtualMenuItem(p)) {
        olcxMenuGenGlobRefsForVirtMethod( ff, rrr);
    }
}

static void genNonVirtualsGlobRefLists(SymbolsMenu *rrr,
                                       FILE *ff,
                                       char *fn) {
    SymbolsMenu *ss;
    ReferencesItem *p;

    // first count if there are some references at all
    for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
    if (ss == NULL) return;
    assert(rrr!=NULL);
    p = &rrr->s;
    assert(p!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->bits.storage]);
    if (! isVirtualMenuItem(p)) {
        for(ss=rrr; ss!=NULL; ss=ss->next) {
            p = &ss->s;
            olcxMenuGenNonVirtualGlobSymList( ff, ss);
        }
    }
}

void splitMenuPerSymbolsAndMap(SymbolsMenu *rrr,
                               void (*fun)(SymbolsMenu *, void *, void *),
                               void *p1,
                               char *p2
                               ) {
    SymbolsMenu *rr, *mp, **ss, *cc, *all;
    ReferencesItem *cs;
    all = NULL;
    rr = rrr;
    while (rr!=NULL) {
        mp = NULL;
        ss= &rr; cs= &rr->s;
        while (*ss!=NULL) {
            cc = *ss;
            if (isSameCxSymbol(&cc->s, cs)) {
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
    if (all!=rrr) {
        ss = &all;
        while (*ss!=rrr && *ss!=NULL)
            ss = &(*ss)->next;
        assert(*ss!=NULL);
        assert (*ss != all);
        *ss = rrr->next;
        rrr->next = all;
    }
}


void generateGlobalReferenceLists(SymbolsMenu *rrr, FILE *ff, char *fn) {
    SymbolsMenu *rr;

    for(rr=rrr; rr!=NULL; rr=rr->next) rr->outOnLine = 0;
    s_symbolListOutputCurrentLine = 1;
    splitMenuPerSymbolsAndMap(rrr, (void (*)(SymbolsMenu *, void *, void *))genNonVirtualsGlobRefLists,
                              ff, fn);
    splitMenuPerSymbolsAndMap(rrr, (void (*)(SymbolsMenu *, void *, void *))genVirtualsGlobRefLists,
                              ff, fn);
}
