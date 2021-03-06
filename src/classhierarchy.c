#include "classhierarchy.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "html.h"
#include "cxref.h"
#include "list.h"

#include "protocol.h"


typedef struct integerList {
    int         i;
    struct integerList   *next;
} IntegerList;



static bitArray tmpChRelevant[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChProcessed[BIT_ARR_DIM(MAX_FILES)];
static bitArray tmpChMarkProcessed[BIT_ARR_DIM(MAX_FILES)];

static S_olSymbolsMenu *tmpVApplClassBackPointersToMenu[MAX_FILES];

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


ClassHierarchyReference *newClassHierarchyReference(int origin, int superClass, ClassHierarchyReference *next) {
    ClassHierarchyReference *p;

    CX_ALLOC(p, ClassHierarchyReference);
    p->ofile = origin;
    p->superClass = superClass;
    p->next = next;

    return p;
}


bool classHierarchyClassNameLess(int c1, int c2) {
    char *nn;
    FileItem *fi1, *fi2;
    int ccc;
    char ttt[MAX_FILE_NAME_SIZE];

    fi1 = fileTable.tab[c1];
    fi2 = fileTable.tab[c2];
    assert(fi1 && fi2);
    // SMART, put interface largest, so they will be at the end
    if (fi2->b.isInterface && ! fi1->b.isInterface)
        return true;
    if (fi1->b.isInterface && ! fi2->b.isInterface)
        return false;
    nn = fi1->name;
    nn = javaGetNudePreTypeName_st(getRealFileNameStatic(nn), options.nestedClassDisplaying);
    strcpy(ttt,nn);
    nn = fi2->name;
    nn = javaGetNudePreTypeName_st(getRealFileNameStatic(nn), options.nestedClassDisplaying);
    ccc = strcmp(ttt,nn);
    if (ccc!=0)
        return ccc<0;
    ccc = strcmp(fi1->name, fi2->name);

    return ccc<0;
}

int classHierarchySupClassNameLess(ClassHierarchyReference *c1, ClassHierarchyReference *c2) {
    return classHierarchyClassNameLess(c1->superClass, c2->superClass);
}

static int markTransitiveRelevantSubsRec(int cind, int pass) {
    FileItem      *fi, *tt;
    ClassHierarchyReference   *s;

    fi = fileTable.tab[cind];
    if (THEBIT(tmpChMarkProcessed,cind))
        return THEBIT(tmpChRelevant,cind);
    SETBIT(tmpChMarkProcessed, cind);
    for(s=fi->inferiorClasses; s!=NULL; s=s->next) {
        tt = fileTable.tab[s->superClass];
        assert(tt);
        // do not descend from class to an
        // interface, because of Object -> interface lapsus ?
        // if ((! fi->b.isInterface) && tt->b.isInterface ) continue;
        // do not mix interfaces in first pass
        //&     if (pass==FIRST_PASS && tt->b.isInterface) continue;
        if (markTransitiveRelevantSubsRec(s->superClass, pass)) {
            //&fprintf(dumpOut,"setting %s relevant\n",fi->name);
            SETBIT(tmpChRelevant, cind);
        }
    }
    return(THEBIT(tmpChRelevant,cind));
}

static void markTransitiveRelevantSubs(int cind, int pass) {
    //&fprintf(dumpOut,"\n PRE checking %s relevant\n",fileTable.tab[cind]->name);
    if (THEBIT(tmpChRelevant,cind)==0) return;
    markTransitiveRelevantSubsRec(cind, pass);
}

void classHierarchyGenInit(void) {
    clearTmpChRelevant();
    clearTmpChProcessed();
}

void setTmpClassBackPointersToMenu(S_olSymbolsMenu *menu) {
    S_olSymbolsMenu *ss;
    clearTmpClassBackPointersToMenu();
    for(ss=menu; ss!=NULL; ss=ss->next) {
        tmpVApplClassBackPointersToMenu[ss->s.vApplClass] = ss;
    }
}

static void genClassHierarchyVerticalBars( FILE *ff, IntegerList *nextbars,
                                           int secondpass) {
    IntegerList *nn;
    if (options.xref2 && options.taskRegime!=RegimeHtmlGenerate) {
        fprintf(ff," %s=\"", PPCA_TREE_DEPS);
    }
    if (nextbars!=NULL) {
        LIST_REVERSE(IntegerList, nextbars);
        for(nn = nextbars; nn!=NULL; nn=nn->next) {
            if (nn->next==NULL) {
                if (secondpass) fprintf(ff,"  +- ");
                else fprintf(ff,"  |  ");
            }
            else if (nn->i) fprintf(ff,"  | ");
            else fprintf(ff,"    ");
        }
        LIST_REVERSE(IntegerList, nextbars);
    }
    if (options.xref2 && options.taskRegime!=RegimeHtmlGenerate) {
        fprintf(ff,"\"");
    }
}


/* ******************************************************************* */
/* ***************************** HTML ******************************** */
/* ******************************************************************* */


static S_olSymbolsMenu *htmlItemInOriginalList(S_olSymbolsMenu *orr, int fInd) {
    S_olSymbolsMenu *rr;
    if (fInd == -1) return(NULL);
    assert(fInd>=0 && fInd<MAX_FILES);
    rr = tmpVApplClassBackPointersToMenu[fInd];
    //& LIST_FIND(S_olSymbolsMenu, fInd, s.vApplClass, orr, rr);
    return(rr);
}

static void htmlCHEmptyIndent( FILE *ff ) {
    fprintf(ff,"    ");
}


static void htmlPrintClassHierarchyLine( FILE *ff, int fInd,
                                         IntegerList *nextbars,
                                         int virtFlag,
                                         S_olSymbolsMenu *itt ) {
    char *cname, *pref1, *pref2, *suf1, *suf2;
    FileItem *fi;
    int cnt;

    fi = fileTable.tab[fInd];
    htmlCHEmptyIndent( ff);
    genClassHierarchyVerticalBars( ff, nextbars, 0);
    fprintf(ff,"\n");

    cname = javaGetNudePreTypeName_st(getRealFileNameStatic(fi->name),
                                      options.nestedClassDisplaying);
    pref1 = pref2 = suf1 = suf2 = "";
    if (fi->b.isInterface) {
        pref2 = "<em>"; suf2 = "</em>";
    }
    if (itt!=NULL && THEBIT(tmpChProcessed,fInd)==0) {
        assert(itt);
        genClassHierarchyItemLinks( ff, itt, virtFlag);
        genClassHierarchyVerticalBars( ff, nextbars, 1);
        if (itt->s.vApplClass == itt->s.vFunClass) {
            pref1 = "<B>"; suf1 = "</B>";
        }
        fprintf(ff,"%s%s%s%s%s", pref1, pref2, cname, suf2, suf1);
        cnt = itt->defRefn+itt->refn;
        if (cnt > 0) {
            fprintf(ff,"\t\t(%d)", cnt);
        }
    } else {
        htmlCHEmptyIndent( ff);
        genClassHierarchyVerticalBars( ff, nextbars, 1);
        fprintf(ff,"%s%s%s%s%s", pref1, pref2, cname, suf2, suf1);
        //&fprintf(ff,"\t\t(0)");
    }
    if (THEBIT(tmpChProcessed,fInd)==1) {
        fprintf(ff, " ---> up");
    }
    fprintf(ff,"\n");
}

static void olcxPrintMenuItemPrefix(FILE *ff, S_olSymbolsMenu *itt,
                                    int selectable) {
    assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
    if (options.server_operation==OLO_CLASS_TREE || options.server_operation==OLO_SHOW_CLASS_TREE) {
        ; //fprintf(ff,"");
    } else if (! selectable) {
        if (options.xref2) fprintf(ff, " %s=2", PPCA_SELECTED);
        else fprintf(ff,"- ");
    } else if (itt!=NULL && itt->selected) {
        if (options.xref2) fprintf(ff, " %s=1", PPCA_SELECTED);
        else fprintf(ff,"+ ");
    } else {
        if (options.xref2) fprintf(ff, " %s=0", PPCA_SELECTED);
        else fprintf(ff,"  ");
    }
    if (itt!=NULL
        && itt->vlevel==1
        && ooBitsGreaterOrEqual(itt->ooBits, OOC_PROFILE_APPLICABLE)) {
        if (options.xref2) fprintf(ff, " %s=1", PPCA_BASE);
        else fprintf(ff,">>");
    } else {
        if (options.xref2) fprintf(ff, " %s=0", PPCA_BASE);
        else fprintf(ff,"  ");
    }
    if (! options.xref2) fprintf(ff," ");
    if (options.server_operation==OLO_CLASS_TREE || options.server_operation==OLO_SHOW_CLASS_TREE) {
        ; //fprintf(ff, "");
    } else if (itt==NULL || (itt->defRefn==0 && itt->refn==0) || !selectable) {
        if (options.xref2) fprintf(ff, " %s=0 %s=0", PPCA_DEF_REFN, PPCA_REFN);
        else fprintf(ff, "  -/-  ");
    } else if (itt->defRefn==0) {
        if (options.xref2) fprintf(ff, " %s=0 %s=%d", PPCA_DEF_REFN, PPCA_REFN, itt->refn);
        else fprintf(ff, "  -/%-3d", itt->refn);
    } else if (itt->refn==0) {
        if (options.xref2) fprintf(ff, " %s=%d %s=0", PPCA_DEF_REFN, itt->defRefn, PPCA_REFN);
        else fprintf(ff, "%3d/-  ", itt->defRefn);
    } else {
        if (options.xref2) fprintf(ff, " %s=%d %s=%d", PPCA_DEF_REFN, itt->defRefn, PPCA_REFN, itt->refn);
        else fprintf(ff, "%3d/%-3d", itt->defRefn, itt->refn);
    }
    if (! options.xref2) fprintf(ff,"    ");
}

static void olcxMenuGenNonVirtualGlobSymList( FILE *ff, S_olSymbolsMenu *ss) {
    char ttt[MAX_CX_SYMBOL_SIZE];

    if (s_symbolListOutputCurrentLine == 1) s_symbolListOutputCurrentLine++; // first line irregularity
    ss->outOnLine = s_symbolListOutputCurrentLine;
    s_symbolListOutputCurrentLine ++ ;
    if (options.xref2) {
        ppcIndent();
        fprintf(ff,"<%s %s=%d", PPC_SYMBOL, PPCA_LINE, ss->outOnLine+SYMBOL_MENU_FIRST_LINE);
        if (ss->s.b.symType!=TypeDefault) {
            fprintf(ff," %s=%s", PPCA_TYPE, typeNamesTable[ss->s.b.symType]);
        }
        olcxPrintMenuItemPrefix(ff, ss, 1);
        sprintfSymbolLinkName(ttt, ss);
        fprintf(ff," %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(ttt), ttt, PPC_SYMBOL);
    } else {
        fprintf(ff,"\n");
        olcxPrintMenuItemPrefix(ff, ss, 1);
        printSymbolLinkName(ff, ss);
        if (ss->s.b.symType != TypeDefault) {
            fprintf(ff,"\t(%s)", typeNamesTable[ss->s.b.symType]);
        }
        //&fprintf(ff," ==%s %o (%s) at %x", ss->s.name, ss->ooBits, refCategoriesName[ss->s.b.category], ss);
    }
}

static void olcxMenuPrintClassHierarchyLine( FILE *ff, int fInd,
                                             IntegerList *nextbars,
                                             int virtFlag,
                                             S_olSymbolsMenu *itt ) {
    char        *cname;
    FileItem  *fi;
    int         yetProcesed, indent;

    yetProcesed = THEBIT(tmpChProcessed,fInd);
    if (options.xref2) {
        ppcIndent();
        fprintf(ff, "<%s %s=%d", PPC_CLASS, PPCA_LINE,
                itt->outOnLine+SYMBOL_MENU_FIRST_LINE);
    }
    olcxPrintMenuItemPrefix(ff, itt, ! yetProcesed);
    if (options.xref2) {
        LIST_LEN(indent, IntegerList, nextbars);
        fprintf(ff, " %s=%d", PPCA_INDENT, indent);
    }
    genClassHierarchyVerticalBars( ff, nextbars, 1);
    fi = fileTable.tab[fInd];
    if (itt!=NULL) {
        assert(itt);
        if (itt->s.vApplClass == itt->s.vFunClass && options.server_operation!=OLO_CLASS_TREE) {
            if (options.xref2) fprintf(ff, " %s=1", PPCA_DEFINITION);
            else fprintf(ff,"*");
        }
        itt->visible = 1;
    }
    if (fi->b.isInterface) {
        if (options.xref2) fprintf(ff, " %s=1", PPCA_INTERFACE);
        else fprintf(ff,"~");
    }
    cname = javaGetNudePreTypeName_st(getRealFileNameStatic(fi->name),
                                      options.nestedClassDisplaying);
    if (options.xref2) {
        if (THEBIT(tmpChProcessed,fInd)) fprintf(ff," %s=1", PPCA_TREE_UP);
        fprintf(ff, " %s=%ld>%s</%s>\n", PPCA_LEN, (unsigned long)strlen(cname), cname, PPC_CLASS);
    } else {
        if (THEBIT(tmpChProcessed,fInd)) fprintf(ff,"(%s) -> up", cname);
        else fprintf(ff,"%s", cname);
        //&if(itt!=NULL)fprintf(ff," %o %s", itt->ooBits, storagesName[itt->s.b.storage]);
        fprintf(ff,"\n");
    }
}


static void descendTheClassHierarchy(FILE *ff,
                                     int vApplCl, int oldvFunCl,
                                     S_olSymbolsMenu *rrr,
                                     int level,
                                     IntegerList *nextbars,
                                     int virtFlag,
                                     int pass
) {
    IntegerList snextbar;
    FileItem *fi;
    ClassHierarchyReference *s, *snext;
    S_olSymbolsMenu *itt;
    int vFunCl;

    fi = fileTable.tab[vApplCl];
    assert(fi!=NULL);
    if (THEBIT(tmpChRelevant,vApplCl)==0) return;
    itt = htmlItemInOriginalList(rrr, vApplCl);

    if (itt == NULL) {
        assert(rrr);
        vFunCl = oldvFunCl;
        // O.K. create new item, so that browse class action will work
        itt = olCreateNewMenuItem(&rrr->s, vApplCl, vFunCl, &s_noPos, UsageNone,
                                  0, 1, 0, UsageNone, 0);
        // insert it into the list, no matter where?
        itt->next = rrr->next;
        rrr->next = itt;
        tmpVApplClassBackPointersToMenu[vApplCl] = itt;
    } else {
        vFunCl = itt->s.vFunClass;
    }

    if (s_symbolListOutputCurrentLine == 1) s_symbolListOutputCurrentLine++; // first line irregularity
    if (itt!=NULL && itt->outOnLine==0) itt->outOnLine = s_symbolListOutputCurrentLine;
    s_symbolListOutputCurrentLine ++;
    if (options.taskRegime==RegimeHtmlGenerate) {
        htmlPrintClassHierarchyLine( ff, vApplCl, nextbars, virtFlag, itt);
    } else {
        olcxMenuPrintClassHierarchyLine( ff, vApplCl, nextbars, virtFlag, itt);
    }

    if (THEBIT(tmpChProcessed,vApplCl)==1) return;
    SETBIT(tmpChProcessed, vApplCl);

    // putting the following in comments makes that for -refnum==1
    // subclasses will not be sorted !
    // also subclasses for on-line resolution would not be sorted!
    LIST_MERGE_SORT(ClassHierarchyReference, fi->inferiorClasses, classHierarchySupClassNameLess);
    s=fi->inferiorClasses;
    while (s!=NULL) {
        assert(fileTable.tab[s->superClass]);
        snext = s->next;
        while (snext!=NULL && THEBIT(tmpChRelevant,snext->superClass)==0) {
            snext = snext->next;
        }
        snextbar = (IntegerList) {.i = (snext!=NULL), .next = nextbars};
        descendTheClassHierarchy(ff, s->superClass, vFunCl, rrr, level+1,
                                 &snextbar, virtFlag, pass);
        s = snext;
    }
}

static bool genThisClassHierarchy(int vApplCl, int oldvFunCl,
                                 FILE *ff,
                                 S_olSymbolsMenu   *rrr,
                                 int virtFlag,
                                 int pass) {
    FileItem *tt,*fi;
    ClassHierarchyReference *s;

    fi = fileTable.tab[vApplCl];
    if (fi==NULL)
        return false;
    if (THEBIT(tmpChProcessed,vApplCl))
        return false;
    if (THEBIT(tmpChRelevant,vApplCl)==0)
        return false;
    // check if you are at the top of a sub-hierarchy
    for(s=fi->superClasses; s!=NULL; s=s->next) {
        tt = fileTable.tab[s->superClass];
        assert(tt);
        if (THEBIT(tmpChRelevant,s->superClass) && THEBIT(tmpChProcessed,s->superClass)==0)
            return false;
    }
    //&fprintf(dumpOut,"getting %s %s\n",top->b.isInterface?"interface":"class",top->name);fflush(dumpOut);
    // yes I am on the top, recursively descent and print all subclasses
    if (pass==FIRST_PASS && fi->b.isInterface)
        return false;
    descendTheClassHierarchy(ff, vApplCl, oldvFunCl, rrr, 0, NULL, virtFlag, pass);
    return true;
}

void genClassHierarchies(FILE *file, S_olSymbolsMenu *menuList,
                         int virtFlag, int passCount) {
    S_olSymbolsMenu *menu;

    // mark the classes where the method is defined and used
    clearTmpChRelevant();
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        assert(fileTable.tab[menu->s.vApplClass]);
        if (menu->visible) {
            SETBIT(tmpChRelevant, menu->s.vApplClass);
            SETBIT(tmpChRelevant, menu->s.vFunClass);
        }
    }
    // now, mark the relevant subtree of class tree
    clearTmpChMarkProcessed();
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        markTransitiveRelevantSubs(menu->s.vFunClass, passCount);
        markTransitiveRelevantSubs(menu->s.vApplClass, passCount);
    }
    // and gen the class subhierarchy
    for(menu=menuList; menu!=NULL; menu=menu->next) {
        genThisClassHierarchy(menu->s.vFunClass, noFileIndex, file, menuList, virtFlag, passCount);
        genThisClassHierarchy(menu->s.vApplClass, noFileIndex, file, menuList, virtFlag, passCount);
    }
}

static void olcxMenuGenGlobRefsForVirtMethod(FILE *ff, S_olSymbolsMenu *rrr) {
    char ln[MAX_HTML_REF_LEN];

    linkNamePrettyPrint(ln,rrr->s.name,MAX_HTML_REF_LEN,SHORT_NAME);
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

static void htmlMarkVisibleAllClassesHavingDefinition( S_olSymbolsMenu *rrr ) {
    S_olSymbolsMenu *rr;
    // put all classes having definition visible
    for(rr=rrr; rr!=NULL; rr=rr->next) {
        if (rr->s.vApplClass==rr->s.vFunClass) rr->visible = 1;
    }
}

static int isVirtualMenuItem(SymbolReferenceItem *p) {
    return (p->b.storage == StorageField
            || p->b.storage == StorageMethod
            || p->b.storage == StorageConstructor);
}

static void genVirtualsGlobRefLists(    S_olSymbolsMenu *rrr,
                                        FILE *ff,
                                        char *fn
                                        ) {
    S_olSymbolsMenu *ss;
    SymbolReferenceItem *p;

    // first count if there are some references at all
    for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
    if (ss == NULL) return;
    assert(rrr!=NULL);
    p = &rrr->s;
    assert(p!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->b.storage]);
    if (isVirtualMenuItem(p)) {
        if (options.taskRegime==RegimeHtmlGenerate) {
            htmlMarkVisibleAllClassesHavingDefinition( rrr);
            htmlGenGlobRefsForVirtMethod( ff, fn, rrr);
        } else {
            olcxMenuGenGlobRefsForVirtMethod( ff, rrr);
        }
    }
}

static void genNonVirtualsGlobRefLists(S_olSymbolsMenu *rrr,
                                       FILE *ff,
                                       char *fn) {
    S_olSymbolsMenu *ss;
    SymbolReferenceItem *p;

    // first count if there are some references at all
    for(ss=rrr; ss!=NULL && ss->visible==0; ss=ss->next) ;
    if (ss == NULL) return;
    assert(rrr!=NULL);
    p = &rrr->s;
    assert(p!=NULL);
    //&fprintf(dumpOut,"storage of %s == %s\n",p->name,storagesName[p->b.storage]);
    if (! isVirtualMenuItem(p)) {
        for(ss=rrr; ss!=NULL; ss=ss->next) {
            p = &ss->s;
            if (options.taskRegime==RegimeHtmlGenerate) {
                htmlGenNonVirtualGlobSymList( ff, fn, p);
            } else {
                olcxMenuGenNonVirtualGlobSymList( ff, ss);
            }
        }
    }
}

void splitMenuPerSymbolsAndMap(S_olSymbolsMenu *rrr,
                               void (*fun)(S_olSymbolsMenu *, void *, void *),
                               void *p1,
                               char *p2
                               ) {
    S_olSymbolsMenu *rr, *mp, **ss, *cc, *all;
    SymbolReferenceItem *cs;
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
        LIST_APPEND(S_olSymbolsMenu, mp, all);
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


void htmlGenerateGlobalReferenceLists(S_olSymbolsMenu *rrr, FILE *ff, char *fn) {
    S_olSymbolsMenu *rr;

    for(rr=rrr; rr!=NULL; rr=rr->next) rr->outOnLine = 0;
    s_symbolListOutputCurrentLine = 1;
    splitMenuPerSymbolsAndMap(rrr, (void (*)(S_olSymbolsMenu *, void *, void *))genNonVirtualsGlobRefLists,
                              ff, fn);
    splitMenuPerSymbolsAndMap(rrr, (void (*)(S_olSymbolsMenu *, void *, void *))genVirtualsGlobRefLists,
                              ff, fn);
}
