#include "cxref.h"

#include "commons.h"
#include "lexer.h"
#include "yylex.h"
#include "classhierarchy.h"
#include "classfilereader.h"
#include "globals.h"
#include "caching.h"
#include "misc.h"
#include "complete.h"
#include "protocol.h"
#include "cxfile.h"
#include "refactory.h"
#include "options.h"
#include "jsemact.h"
#include "editor.h"
#include "main.h"               /* For searchDefaultOptionsFile() */
#include "reftab.h"
#include "characterreader.h"
#include "symbol.h"
#include "list.h"
#include "filedescriptor.h"
#include "fileio.h"

#include "log.h"
#include "utils.h"


typedef struct referencesChangeData {
    char			*linkName;
    int				fnum;
    struct symbol	*cclass;
    int				category;
    int				cxMemBegin;
    int				cxMemEnd;
} S_referencesChangeData;


#define OLCX_USER_RESERVE 30

/* ************************************************************** */

#include "olcxtab.h"

/* *********************************************************************** */

void fillSymbolRefItem(SymbolReferenceItem *symbolRefItem, char *name,
                       unsigned fileHash, int vApplClass, int vFunClass) {
    symbolRefItem->name = name;
    symbolRefItem->fileHash = fileHash;
    symbolRefItem->vApplClass = vApplClass;
    symbolRefItem->vFunClass = vFunClass;
    symbolRefItem->refs = NULL;
    symbolRefItem->next = NULL;
}

void fillSymbolRefItemBits(SymbolReferenceItemBits *symbolRefItemBits, unsigned symType,
                           unsigned storage, unsigned scope, unsigned accessFlags,
                           unsigned category) {
    symbolRefItemBits->symType = symType;
    symbolRefItemBits->storage = storage;
    symbolRefItemBits->scope = scope;
    symbolRefItemBits->accessFlags = accessFlags;                \
    symbolRefItemBits->category = category;
}

void fillReference(Reference *reference, UsageBits usage, Position position, Reference *next) {
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
}


void fillSymbolsMenu(SymbolsMenu *symbolsMenu,
                     struct symbolReferenceItem s,
                     char selected,
                     char visible,
                     unsigned ooBits,
                     char olUsage,
                     short int vlevel,
                     short int refn,
                     short int defRefn,
                     char defUsage,
                     struct position defpos,
                     int outOnLine,
                     struct editorMarkerList *markers,	/* for refactory only */
                     struct SymbolsMenu *next
) {
    symbolsMenu->s = s;
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


void fillUsageBits(UsageBits *usageBits, unsigned base, unsigned requiredAccess) {
    usageBits->base = base;
    usageBits->requiredAccess = requiredAccess;
}


int olcxReferenceInternalLessFunction(Reference *r1, Reference *r2) {
    return SORTED_LIST_LESS(r1, (*r2));
}

bool olSymbolRefItemLess(SymbolReferenceItem *s1, SymbolReferenceItem *s2) {
    int cmp;
    cmp = strcmp(s1->name, s2->name);
    if (cmp < 0)
        return true;
    else if (cmp > 0)
        return false;
    if (s1->vFunClass < s2->vFunClass)
        return true;
    else if (s1->vFunClass > s2->vFunClass)
        return false;
    if (s1->vApplClass < s2->vApplClass)
        return true;
    else if (s1->vApplClass > s2->vApplClass)
        return false;
    if (s1->b.symType < s2->b.symType)
        return true;
    else if (s1->b.symType > s2->b.symType)
        return false;
    if (s1->b.storage < s2->b.storage)
        return true;
    else if (s1->b.storage > s2->b.storage)
        return false;
    if (s1->b.category < s2->b.category)
        return true;
    else if (s1->b.category > s2->b.category)
        return false;
    return false;
}

static bool olSymbolMenuLess(SymbolsMenu *s1, SymbolsMenu *s2) {
    return olSymbolRefItemLess(&s1->s, &s2->s);
}

static char *olcxStringCopy(char *string) {
    int length;
    char *copy;
    length = strlen(string);
    copy = olcx_memory_allocc(length+1, sizeof(char));
    strcpy(copy, string);
    return copy;
}


SymbolsMenu *olCreateNewMenuItem(SymbolReferenceItem *symbol, int vApplClass, int vFunCl,
                                     Position *defpos, int defusage,
                                     int selected, int visible,
                                     unsigned ooBits, int olusage, int vlevel) {
    SymbolsMenu *symbolsMenu;
    SymbolReferenceItem refItem;
    char *allocatedNameCopy;

    allocatedNameCopy = olcxStringCopy(symbol->name);

    fillSymbolRefItem(&refItem, allocatedNameCopy,
                                cxFileHashNumber(allocatedNameCopy),
                                vApplClass, vFunCl);
    refItem.b = symbol->b;

    symbolsMenu = olcx_alloc(sizeof(SymbolsMenu));
    fillSymbolsMenu(symbolsMenu, refItem, selected, visible, ooBits, olusage,
                       vlevel, 0, 0, defusage, *defpos, 0, NULL, NULL);
    return symbolsMenu;
}

SymbolsMenu *olAddBrowsedSymbol(SymbolReferenceItem *sym, SymbolsMenu **list,
                                int selected, int visible, unsigned ooBits,
                                int olusage, int vlevel,
                                Position *defpos, int defusage) {
    SymbolsMenu *rr, **place, ddd;

    fillSymbolsMenu(&ddd, *sym, 0,0,0, olusage, vlevel,0,0, UsageNone,noPosition,0, NULL, NULL);
    SORTED_LIST_PLACE3(place, SymbolsMenu, (&ddd), list, olSymbolMenuLess);
    rr = *place;
    if (*place==NULL || olSymbolMenuLess(&ddd, *place)) {
        assert(sym);
        rr = olCreateNewMenuItem(sym, sym->vApplClass, sym->vFunClass, defpos, defusage,
                                 selected, visible, ooBits,
                                 olusage, vlevel);
        LIST_CONS(rr,(*place));
        log_trace(":adding browsed symbol '%s'", sym->name);
    }
    return rr;
}

void renameCollationSymbols(SymbolsMenu *sss) {
    int                 len,len1;
    char                *nn, *cs;
    assert(sss);
    for (SymbolsMenu *ss=sss; ss!=NULL; ss=ss->next) {
        cs = strchr(ss->s.name, LINK_NAME_COLLATE_SYMBOL);
        if (cs!=NULL && ss->s.b.symType==TypeCppCollate) {
            len = strlen(ss->s.name);
            assert(len>=2);
            nn = olcx_memory_allocc(len-1, sizeof(char));
            len1 = cs-ss->s.name;
            strncpy(nn, ss->s.name, len1);
            strcpy(nn+len1, cs+2);
            //&fprintf(dumpOut, "renaming %s to %s\n", ss->s.name, nn);
            olcx_memory_free(ss->s.name, len+1);
            ss->s.name = nn;
        }
    }
}


/* *********************************************************************** */


/*
  void deleteFromRefList(void *p) {
  S_reference **pp, *ff;
  pp = (S_reference **) p;
  ff = *pp;
  *pp = (*pp)->next;
  CX_FREE(ff);
  }
*/


Reference **addToRefList(Reference **list,
                         UsageBits *usageP,
                         Position *pos) {
    Reference *rr, **place;
    Reference reference;

    fillReference(&reference, *usageP, *pos, NULL);
    SORTED_LIST_PLACE2(place,reference,list);
    if (*place==NULL || SORTED_LIST_NEQ((*place),reference)
        || options.server_operation==OLO_EXTRACT) {
        CX_ALLOC(rr, Reference);
        fillReference(rr, *usageP, *pos, NULL);
        LIST_CONS(rr, (*place));
    } else {
        assert(*place);
        (*place)->usage = *usageP;
    }
    return place;
}


Reference *duplicateReference(Reference *r) {
    // this is used in extract x=x+2; to re-arrange order of references
    // i.e. usage must be first, lValue second.
    Reference *rr;
    r->usage = NO_USAGE;
    CX_ALLOC(rr, Reference);
    *rr = *r;
    r->next = rr;
    return rr;
}


static void getSymbolCxrefCategories(Symbol *symbol,
                                     int *p_category,
                                     int *p_scope,
                                     int *p_storage) {
    int category, scope, storage;
    category = CategoryLocal; scope = ScopeAuto; storage=StorageAuto;
    /* default */
    if (symbol->bits.symbolType==TypeDefault) {
        storage = symbol->bits.storage;
        if (    symbol->bits.storage==StorageExtern
                ||  symbol->bits.storage==StorageDefault
                ||  symbol->bits.storage==StorageTypedef
                ||  symbol->bits.storage==StorageField
                ||  symbol->bits.storage==StorageMethod
                ||  symbol->bits.storage==StorageConstructor
                ||  symbol->bits.storage==StorageStatic
                ||  symbol->bits.storage==StorageThreadLocal
                ) {
            if (symbol->linkName[0]==' ' && symbol->linkName[1]==' ') {
                // a special symbol local linkname
                category = CategoryLocal;
            } else {
                category = CategoryGlobal;
            }
            scope = ScopeGlobal;
        }
    }
    /* enumeration constants */
    if (symbol->bits.symbolType==TypeDefault && symbol->bits.storage==StorageConstant) {
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    /* struct, union, enum */
    if ((symbol->bits.symbolType==TypeStruct||symbol->bits.symbolType==TypeUnion||symbol->bits.symbolType==TypeEnum)){
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    /* macros */
    if (symbol->bits.symbolType == TypeMacro) {
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->bits.symbolType == TypeLabel) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }
    if (symbol->bits.symbolType == TypeCppIfElse) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }
    if (symbol->bits.symbolType == TypeCppInclude) {
        category = CategoryGlobal; scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->bits.symbolType == TypeCppCollate) {
        category = CategoryGlobal; scope = ScopeGlobal; storage=StorageExtern;
    }
    if (symbol->bits.symbolType == TypeYaccSymbol) {
        category = CategoryLocal; scope = ScopeFile; storage=StorageStatic;
    }
    /* JAVA packages */
    if (symbol->bits.symbolType == TypePackage) {
        category = CategoryGlobal;  scope = ScopeGlobal; storage=StorageExtern;
    }

    *p_category = category;
    *p_scope = scope;
    *p_storage = storage;
}


static void setClassTreeBaseType(S_classTreeData *ct, Symbol *p) {
    Symbol        *rtcls;
    TypeModifier *tt;
    assert(s_javaObjectSymbol && s_javaObjectSymbol->u.structSpec);
    assert(ct);
    //&fprintf(dumpOut,"!looking for result of %s\n",p->linkName);fflush(dumpOut);
    ct->baseClassFileIndex = s_javaObjectSymbol->u.structSpec->classFile;
    if (p->bits.symbolType == TypeStruct) {
        assert(p->u.structSpec);
        ct->baseClassFileIndex = p->u.structSpec->classFile;
    } else if (p->bits.symbolType == TypeDefault && p->bits.storage!=StorageConstructor) {
        tt = p->u.typeModifier;
        assert(tt);
        if (tt->kind == TypeFunction) tt = tt->next;
        assert(tt);
        if (tt->kind == TypeStruct) {
            rtcls = tt->u.t;
            assert(rtcls!=NULL && rtcls->bits.symbolType==TypeStruct
                   && rtcls->u.structSpec!=NULL);
            //&fprintf(dumpOut,"!resulting class is %s\n",rtcls->linkName);fflush(dumpOut);
            ct->baseClassFileIndex = rtcls->u.structSpec->classFile;
        }
    }
}

Reference *addSpecialFieldReference(char *name,int storage, int fnum,
                                    Position *p, int usage){
    Symbol        ss;
    Reference     *res;

    ss = makeSymbolWithBits(name, name, *p, AccessDefault, TypeDefault, storage);
    res = addCxReference(&ss, p, usage, fnum, fnum);

    return res;
}

static bool isEnclosingClass(int enclosedClass, int enclosingClass) {
    int slow;
    int cc = 0;

    for (int i = slow = enclosedClass; i!=noFileIndex && i!=-1; i=fileTable.tab[i]->directEnclosingInstance) {
        assert(fileTable.tab[i]);
        if (i == enclosingClass)
            return true;
        // this loop looks very suspect, I prefer to put here a loop check
        if (cc==0) {
            cc = !cc;
        } else {
            assert(slow != i);
            slow = fileTable.tab[slow]->directEnclosingInstance;
        }
    }
    return false;
}

bool isStrictlyEnclosingClass(int enclosedClass, int enclosingClass) {
    if (enclosedClass == enclosingClass)
        return false;
    return isEnclosingClass(enclosedClass, enclosingClass);
}

// TODO, all this stuff should be done differently!
static void changeFieldRefUsages(SymbolReferenceItem *ri, void *rrcd) {
    S_referencesChangeData *rcd;
    SymbolReferenceItem ddd;

    rcd = (S_referencesChangeData*) rrcd;
    fillSymbolRefItem(&ddd,rcd->linkName,
                                cxFileHashNumber(rcd->linkName),
                                noFileIndex, noFileIndex);
    fillSymbolRefItemBits(&ddd.b, TypeDefault, StorageField,
                           ScopeFile, AccessDefault, rcd->category);
    if (isSameCxSymbol(ri, &ddd)) {
        //&sprintf(tmpBuff, "checking %s <-> %s, %d,%d", ri->name, rcd->linkName, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
        for (Reference *rr = ri->refs; rr!=NULL; rr=rr->next) {
            //&sprintf(tmpBuff, "checking %d,%d %d,%d,%d", rr->position.file, rcd->fnum, rr, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
            if (rr->position.file == rcd->fnum &&  /* I think it is used only for Java */
                DM_IS_BETWEEN(cxMemory,rr,rcd->cxMemBegin,rcd->cxMemEnd)) {
                //&sprintf(tmpBuff, "checking %d,%d %d,%d,%d", rr->position.file, rcd->fnum, rr, rcd->cxMemBegin,rcd->cxMemEnd);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
                switch(rr->usage.base) {
                case UsageMaybeThis:
                    assert(rcd->cclass->u.structSpec);
                    if (isEnclosingClass(rcd->cclass->u.structSpec->classFile, ri->vFunClass)) {
                        rr->usage.base = UsageMaybeThisInClassOrMethod;
                    }
                    break;
                case UsageMaybeQualifiedThis:
                    assert(rcd->cclass->u.structSpec);
                    if (isEnclosingClass(rcd->cclass->u.structSpec->classFile, ri->vFunClass)) {
                        rr->usage.base = UsageMaybeQualifThisInClassOrMethod;
                    }
                    break;
                case UsageNotFQType: rr->usage.base = UsageNotFQTypeInClassOrMethod;
                    //&sprintf(tmpBuff, "reseting %d:%d:%d at %d", rr->position.file, rr->position.line, rr->position.col, rr);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
                    break;
                case UsageNotFQField: rr->usage.base = UsageNotFQFieldInClassOrMethod;
                    break;
                case UsageNonExpandableNotFQTName: rr->usage.base = UsageNonExpandableNotFQTNameInClassOrMethod;
                    break;
                case UsageLastUseless: rr->usage.base = UsageLastUselessInClassOrMethod;
                    break;
                case UsageOtherUseless:
                    //& rr->usage.base = UsageOtherUselessInMethod;
                    break;
                case UsageLastUselessInClassOrMethod:
                case UsageNotFQFieldInClassOrMethod:
                case UsageNotFQTypeInClassOrMethod:
                case UsageMaybeQualifThisInClassOrMethod:
                case UsageMaybeThisInClassOrMethod:
                    // do not care if it is yet requalified
                    break;
                default:
                    break;
                }
            }
        }
    }
}

static void fillReferencesChangeData(S_referencesChangeData *referencesChangeData, char *linkName, int fnum,
                                      Symbol *cclass, int category, int memBegin, int memEnd) {
    referencesChangeData->linkName = linkName;
    referencesChangeData->fnum = fnum;
    referencesChangeData->cclass = cclass;
    referencesChangeData->category = category;
    referencesChangeData->cxMemBegin = memBegin;
    referencesChangeData->cxMemEnd = memEnd;
}

void changeMethodReferencesUsages(char *linkName, int category, int fnum,
                                  Symbol *cclass){
    S_referencesChangeData rr;
    fillReferencesChangeData(&rr, linkName, fnum, cclass, category,
                              s_cps.cxMemoryIndexAtMethodBegin,
                              s_cps.cxMemoryIndexAtMethodEnd);
    refTabMap2(&referenceTable, changeFieldRefUsages, &rr);
}

void changeClassReferencesUsages(char *linkName, int category, int fnum,
                                 Symbol *cclass){
    S_referencesChangeData rr;
    fillReferencesChangeData(&rr, linkName, fnum, cclass, category,
                              s_cps.cxMemoryIndexAtClassBeginning,
                              s_cps.cxMemoryIndexAtClassEnd);
    refTabMap2(&referenceTable, changeFieldRefUsages, &rr);
}

Reference * getDefinitionRef(Reference *rr) {
    Reference *res = NULL;

    for (Reference *r=rr; r!=NULL; r=r->next) {
        if (r->usage.base==UsageDefined || r->usage.base==UsageOLBestFitDefined) {
            res = r;
        }
        if (r->usage.base==UsageDeclared && res==NULL)
            res = r;
    }
    return res;
}

// used only with OLO_GET_SYMBOL_TYPE;
static void setOlSymbolTypeForPrint(Symbol *p) {
    int             size, len;
    TypeModifier *tt;
    size = COMPLETION_STRING_SIZE;
    s_olSymbolType[0]=0;
    s_olSymbolClassType[0]=0;
    if (p->bits.symbolType == TypeDefault) {
        tt = p->u.typeModifier;
        if (tt!=NULL && tt->kind==TypeFunction) tt = tt->next;
        typeSPrint(s_olSymbolType, &size, tt, "", ' ', 0, 1, LONG_NAME, NULL);
        if (tt->kind == TypeStruct && tt->u.t!=NULL) {
            strcpy(s_olSymbolClassType, tt->u.t->linkName);
            assert(strlen(s_olSymbolClassType)+1 < COMPLETION_STRING_SIZE);
        }
        // remove pending spaces
        len = strlen(s_olSymbolType);
        while (len > 0 && s_olSymbolType[len-1] == ' ') len--;
        s_olSymbolType[len]=0;
    }
}

typedef struct availableRefactoring {
    bool available;
    char *option;
} AvailableRefactoring;


static AvailableRefactoring availableRefactorings[MAX_AVAILABLE_REFACTORINGS];

void initAvailableRefactorings(void) {
    for (int i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
        availableRefactorings[i].available = false;
        availableRefactorings[i].option = "";
    }
}

static void setOlAvailableRefactorings(Symbol *p, SymbolsMenu *mmi, int usage) {
    char *opt;
    if (strcmp(p->linkName, LINK_NAME_UNIMPORTED_QUALIFIED_ITEM)==0) {
        availableRefactorings[PPC_AVR_ADD_TO_IMPORT].available = 1;
        return;
    }
    switch (p->bits.symbolType) {
    case TypePackage:
        availableRefactorings[PPC_AVR_RENAME_PACKAGE].available = true;
        CX_ALLOCC(opt, strlen(mmi->s.name)+1, char);
        strcpy(opt, mmi->s.name);
        javaDotifyFileName(opt);
        availableRefactorings[PPC_AVR_RENAME_PACKAGE].option = opt;
        break;
    case TypeStruct:
        if (LANGUAGE(LANG_JAVA)) {
            if (IS_DEFINITION_USAGE(usage)) {
                availableRefactorings[PPC_AVR_RENAME_CLASS].available = true;
                availableRefactorings[PPC_AVR_MOVE_CLASS].available = true;
                availableRefactorings[PPC_AVR_MOVE_CLASS_TO_NEW_FILE].available = true;
                //& availableRefactorings[PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE].available = true;
                availableRefactorings[PPC_AVR_EXPAND_NAMES].available = true;
                availableRefactorings[PPC_AVR_REDUCE_NAMES].available = true;
            }
        } else {
            availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = true;
        }
        break;
    case TypeMacroArg:
        availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = true;
        break;
    case TypeLabel:
        availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = true;
        break;
    case TypeMacro:
        availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = true;
        availableRefactorings[PPC_AVR_ADD_PARAMETER].available = true;
        availableRefactorings[PPC_AVR_ADD_PARAMETER].option = "macro";
        availableRefactorings[PPC_AVR_DEL_PARAMETER].available = true;
        availableRefactorings[PPC_AVR_DEL_PARAMETER].option = "macro";
        availableRefactorings[PPC_AVR_MOVE_PARAMETER].available = true;
        availableRefactorings[PPC_AVR_MOVE_PARAMETER].option = "macro";
        break;
    default:
        if (p->bits.storage != StorageConstructor) {
            availableRefactorings[PPC_AVR_RENAME_SYMBOL].available = true;
        }
        if (p->u.typeModifier->kind == TypeFunction || p->u.typeModifier->kind == TypeMacro) {
            availableRefactorings[PPC_AVR_ADD_PARAMETER].available = true;
            availableRefactorings[PPC_AVR_DEL_PARAMETER].available = true;
            availableRefactorings[PPC_AVR_MOVE_PARAMETER].available = true;
        }
        if (p->bits.storage == StorageField) {
            if (IS_DEFINITION_USAGE(usage)) {
                if (p->bits.access & AccessStatic) {
                    availableRefactorings[PPC_AVR_MOVE_STATIC_FIELD].available = true;
                } else {
                    // TODO! restrict this better
                    availableRefactorings[PPC_AVR_PULL_UP_FIELD].available = true;
                    availableRefactorings[PPC_AVR_PUSH_DOWN_FIELD].available = true;
                }
                availableRefactorings[PPC_AVR_MOVE_FIELD].available = true;
                availableRefactorings[PPC_AVR_ENCAPSULATE_FIELD].available = true;
                availableRefactorings[PPC_AVR_SELF_ENCAPSULATE_FIELD].available = true;
            }
        }
        if (p->bits.storage == StorageMethod || p->bits.storage == StorageConstructor) {
            if (IS_DEFINITION_USAGE(usage)) {
                //&availableRefactorings[PPC_AVR_EXPAND_NAMES].available = true;
                //&availableRefactorings[PPC_AVR_REDUCE_NAMES].available = true;
                if (p->bits.access & AccessStatic) {
                    availableRefactorings[PPC_AVR_MOVE_STATIC_METHOD].available = true;
                    if (p->bits.storage == StorageMethod) {
                        availableRefactorings[PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC].available = true;
                    }
                } else {
                    if (p->bits.storage == StorageMethod) {
                        // TODO! some restrictions
                        availableRefactorings[PPC_AVR_PULL_UP_METHOD].available = true;
                        availableRefactorings[PPC_AVR_PUSH_DOWN_METHOD].available = true;
                        availableRefactorings[PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC].available = true;
                    }
                }
            }
        }
        break;
    }
}

static void olGetAvailableRefactorings(void) {
    int count;

    if (! options.xref2) {
        fprintf(communicationChannel,"* refactoring list not available in C-xrefactory-I");
        return;
    }

    count = 0;
    for (int i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
        count += (availableRefactorings[i].available);
    }
    if (count==0) availableRefactorings[PPC_AVR_SET_MOVE_TARGET].available = true;
    if (options.editor == EDITOR_EMACS) {
        availableRefactorings[PPC_AVR_UNDO].available = true;
#if ZERO
        if (LANGUAGE(LANG_JAVA)) {
            s_availableRefactorings[PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS].available = true;
        }
#endif
    }
    if (options.olCursorPos != options.olMarkPos) {
        // region selected, TODO!!! some more prechecks for extract method
        if (LANGUAGE(LANG_JAVA)) {
            availableRefactorings[PPC_AVR_EXTRACT_METHOD].available = true;
        } else {
            availableRefactorings[PPC_AVR_EXTRACT_FUNCTION].available = true;
        }
        if (! LANGUAGE(LANG_JAVA)) {
            availableRefactorings[PPC_AVR_EXTRACT_MACRO].available = true;
        }
    }
    ppcBegin(PPC_AVAILABLE_REFACTORINGS);
    for (int i=0; i<MAX_AVAILABLE_REFACTORINGS; i++) {
        if (availableRefactorings[i].available) {
            ppcValueRecord(PPC_INT_VALUE, i, availableRefactorings[i].option);
        }
    }
    ppcEnd(PPC_AVAILABLE_REFACTORINGS);
}


static bool olcxOnlyParseNoPushing(int opt) {
    return opt==OLO_GLOBAL_UNUSED || opt==OLO_LOCAL_UNUSED;
}


/* ********************************************************************* */
/* ********************************************************************* */
/* default vappClass == vFunClass == s_noneFileIndex !!!!!!!             */
/*                                                                       */
Reference *addCxReferenceNew(Symbol *symbol, Position *pos, UsageBits *usage,
                                int vFunCl, int vApplCl) {
    int                  index;
    int                  category;
    int                  scope;
    int                  storage;
    int                  defaultUsage;
    char                *linkName;
    Reference            rr;
    Reference **         place;
    Position *           defpos;
    SymbolReferenceItem *pp;
    SymbolReferenceItem *memb;
    SymbolReferenceItem  ppp;
    SymbolsMenu *        mmi;
    ReferenceTable *     reftab;
    int                  usage_base;

    usage_base = usage->base;
    // do not record references during prescanning
    // this is because of cxMem overflow during prescanning (for ex. with -html)
    // TODO: So is this relevant now that HTML is gone?
    if (s_javaPreScanOnly)
        return NULL;
    if (symbol->linkName == NULL)
        return NULL;
    if (* symbol->linkName == 0)
        return NULL;
    if (symbol == &s_errorSymbol || symbol->bits.symbolType==TypeError)
        return NULL;
    if (pos->file == noFileIndex)
        return NULL;

    assert(pos->file<MAX_FILES);
    assert(fileTable.tab[pos->file]);

    getSymbolCxrefCategories(symbol, &category, &scope, &storage);

    log_trace("adding reference on %s(%d,%d) at %d,%d,%d (%s) (%s) (%s)", symbol->linkName,
              vFunCl,vApplCl, pos->file, pos->line, pos->col, category==CategoryGlobal?"Global":"Local",
              usageEnumName[usage_base], storageEnumName[symbol->bits.storage]);
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        if (options.server_operation == OLO_EXTRACT) {
            if (inputFileNumber != currentFile.lexBuffer.buffer.fileNumber)
                return NULL;
        } else {
            if (category==CategoryGlobal && symbol->bits.symbolType!=TypeCppInclude && options.server_operation!=OLO_TAG_SEARCH) {
                // do not load references if not the currently edited file
                if (olOriginalFileNumber != pos->file && options.noIncludeRefs)
                    return NULL;
                // do not load references if current file is an
                // included header, they will be reloaded from ref file
                //&fprintf(dumpOut,"%s comm %d\n", fileTable.tab[pos->file]->name, fileTable.tab[pos->file]->b.commandLineEntered);
            }
        }
    }
    if (options.taskRegime == RegimeXref) {
        if (category == CategoryLocal)
            return NULL; /* dont cxref local symbols */
        if (!fileTable.tab[pos->file]->b.cxLoading)
            return NULL;
    }
    reftab = &referenceTable;
    fillSymbolRefItem(&ppp, symbol->linkName, 0, vApplCl, vFunCl);
    fillSymbolRefItemBits(&ppp.b, symbol->bits.symbolType, storage, scope,
                          symbol->bits.access, category);
    if (options.taskRegime==RegimeEditServer && options.server_operation==OLO_TAG_SEARCH && options.tagSearchSpecif==TSS_FULL_SEARCH) {
        fillUsageBits(&rr.usage, usage_base, 0);
        fillReference(&rr, rr.usage, *pos, NULL);
        searchSymbolCheckReference(&ppp, &rr);
        return NULL;
    }
    if (!refTabIsMember(reftab, &ppp, &index, &memb)) {
        log_trace("allocating '%s'", symbol->linkName);
        CX_ALLOC(pp, SymbolReferenceItem);
        CX_ALLOCC(linkName, strlen(symbol->linkName)+1, char);
        strcpy(linkName, symbol->linkName);
        fillSymbolRefItem(pp, linkName, cxFileHashNumber(linkName), vApplCl, vFunCl);
        fillSymbolRefItemBits(&pp->b, symbol->bits.symbolType, storage, scope,
                              symbol->bits.access, category);
        refTabSet(reftab, pp, index);
        memb = pp;
    } else {
        // at least reset some maybe new informations
        // sometimes classes were added from directory listing,
        // without knowing if it is an interface or not
        memb->b.accessFlags |= symbol->bits.access;
    }
    /*  category = reftab->category; */
    place = addToRefList(&memb->refs,usage,pos);
    //&fprintf(dumpOut,"checking %s(%d),%d,%d <-> %s(%d),%d,%d == %d(%d), usage == %d, %s\n", fileTable.tab[s_cxRefPos.file]->name, s_cxRefPos.file, s_cxRefPos.line, s_cxRefPos.col, fileTable.tab[pos->file]->name, pos->file, pos->line, pos->col, memcmp(&s_cxRefPos, pos, sizeof(Position)), positionsAreEqual(s_cxRefPos, *pos), usage_base, symbol->linkName);

    if (options.taskRegime == RegimeEditServer
        && positionsAreEqual(s_cxRefPos, *pos)
        && usage_base<UsageMaxOLUsages) {
        if (symbol->linkName[0] == ' ') {  // special symbols for internal use!
            if (strcmp(symbol->linkName, LINK_NAME_UNIMPORTED_QUALIFIED_ITEM)==0) {
                if (options.server_operation == OLO_GET_AVAILABLE_REFACTORINGS) {
                    setOlAvailableRefactorings(symbol, NULL, usage_base);
                }
            }
        } else {
            /* an on - line cxref action ?*/
            //&fprintf(dumpOut,"!got it %s !!!!!!!\n", memb->name);
            s_olstringServed = 1;       /* olstring will be served */
            s_olstringUsage = usage_base;
            assert(currentUserData && currentUserData->browserStack.top);
            olSetCallerPosition(pos);
            defpos = &noPosition;
            defaultUsage = NO_USAGE.base;
            if (symbol->bits.symbolType==TypeMacro && ! options.exactPositionResolve) {
                // a hack for macros
                defpos = &symbol->pos;
                defaultUsage = UsageDefined;
            }
            if (defpos->file!=noFileIndex)
                log_trace("getting definition position of %s at line %d", symbol->name, defpos->line);
            if (! olcxOnlyParseNoPushing(options.server_operation)) {
                mmi = olAddBrowsedSymbol(memb,&currentUserData->browserStack.top->hkSelectedSym,
                                         1,1,0,usage_base,0, defpos, defaultUsage);
                // hack added for EncapsulateField
                // to determine whether there is already definitions of getter/setter
                if (IS_DEFINITION_USAGE(usage_base)) {
                    mmi->defpos = *pos;
                    mmi->defUsage = usage_base;
                }
                if (options.server_operation == OLO_CLASS_TREE
                    && LANGUAGE(LANG_JAVA)) {
                    setClassTreeBaseType(&currentUserData->classTree, symbol);
                }
                if (options.server_operation == OLO_GET_SYMBOL_TYPE) {
                    setOlSymbolTypeForPrint(symbol);
                }
                if (options.server_operation == OLO_GET_AVAILABLE_REFACTORINGS) {
                    setOlAvailableRefactorings(symbol, mmi, usage_base);
                }
            }
        }
    }

    /* Test for available space */
    assert(options.taskRegime);
    if (options.taskRegime==RegimeXref) {
        if (!(DM_ENOUGH_SPACE_FOR(cxMemory, CX_SPACE_RESERVE))) {
            longjmp(cxmemOverflow, LONGJUMP_REASON_REFERENCE_OVERFLOW);
        }
    }

    assert(place);
    log_trace("returning %x == %s %s:%d", *place, usageEnumName[(*place)->usage.base],
              fileTable.tab[(*place)->position.file]->name, (*place)->position.line);
    return *place;
}

Reference * addCxReference(Symbol *symbol, Position *pos, Usage usage, int vFunClass, int vApplClass) {
    UsageBits ub;
    fillUsageBits(&ub, usage, MIN_REQUIRED_ACCESS);
    return addCxReferenceNew(symbol, pos, &ub, vFunClass, vApplClass);
}

void addTrivialCxReference(char *name, int symType, int storage, Position *pos, int usage) {
    Symbol symbol;

    fillSymbol(&symbol, name, name, *pos);
    fillSymbolBits(&symbol.bits, AccessDefault, symType, storage);
    addCxReference(&symbol, pos, usage, noFileIndex, noFileIndex);
}

void addClassTreeHierarchyReference(int fnum, Position *p, int usage) {
    addSpecialFieldReference(LINK_NAME_CLASS_TREE_ITEM,StorageMethod,
                             fnum, p, usage);
}

void addCfClassTreeHierarchyRef(int fnum, int usage) {
    Position p = makePosition(fnum, 0, 0);
    addClassTreeHierarchyReference(fnum, &p, usage);
}

/* ***************************************************************** */

static int isSmallerOrEqClassR(int inf, int sup, int level) {
    assert(level>0);
    if (inf == sup)
        return level;
    assert(fileTable.tab[inf]);
    for (ClassHierarchyReference *p=fileTable.tab[inf]->superClasses; p!=NULL; p=p->next) {
        if (p->superClass == sup) {
            //&fprintf(dumpOut,"return 1\n");
            return level+1;
        }
    }
    for (ClassHierarchyReference *p=fileTable.tab[inf]->superClasses; p!=NULL; p=p->next) {
        int smallerLevel = isSmallerOrEqClassR(p->superClass, sup, level+1);
        if (smallerLevel) {
            return smallerLevel;
        }
    }
    return 0;
}

bool isSmallerOrEqClass(int inf, int sup) {
    return isSmallerOrEqClassR(inf, sup, 1) != 0;
}

void olcxFreeReferences(Reference *r) {
    Reference *tmp;
    while (r!=NULL) {
        tmp = r->next;
        olcx_memory_free((r), sizeof(Reference));
        r = tmp;
    }
}

static void olcxFreeCompletion(S_olCompletion *r) {
    olcx_memory_free(r->name, strlen(r->name)+1);
    if (r->fullName!=NULL)
        olcx_memory_free(r->fullName, strlen(r->fullName)+1);
    if (r->vclass!=NULL)
        olcx_memory_free(r->vclass, strlen(r->vclass)+1);
    if (r->category == CategoryGlobal) {
        assert(r->sym.name);
        olcx_memory_free(r->sym.name, strlen(r->sym.name)+1);
    }
    olcx_memory_free(r, sizeof(S_olCompletion));
}


static void olcxFreeCompletions(S_olCompletion *r ) {
    S_olCompletion *tmp;

    while (r!=NULL) {
        tmp = r->next;
        olcxFreeCompletion(r);
        r = tmp;
    }
}

SymbolsMenu *olcxFreeSymbolMenuItem(SymbolsMenu *ll) {
    int nlen;
    SymbolsMenu *tt;
    nlen = strlen(ll->s.name);
    olcx_memory_free(ll->s.name, nlen+1);
    olcxFreeReferences(ll->s.refs);
    tt = ll->next;
    olcx_memory_free(ll, sizeof(*ll));
    ll = tt;
    return ll;
}


void olcxFreeResolutionMenu(SymbolsMenu *sym ) {
    SymbolsMenu *ll;

    ll = sym;
    while (ll!=NULL) {
        ll = olcxFreeSymbolMenuItem(ll);
    }
}

static void deleteOlcxRefs(OlcxReferences **rrefs, OlcxReferencesStack *stack) {
    OlcxReferences    *refs;
    refs = *rrefs;
    /*fprintf(ccOut,": pop 2\n"); fflush(ccOut);*/
    olcxFreeReferences(refs->references);
    olcxFreeCompletions(refs->completions);
    olcxFreeResolutionMenu(refs->hkSelectedSym);
    olcxFreeResolutionMenu(refs->menuSym);
    /*fprintf(ccOut,": pop 3\n"); fflush(ccOut);*/
    // if deleting second entry point, update it
    if (refs==stack->top) {
        stack->top = refs->previous;
    }
    // this is useless, but one never knows
    if (refs==stack->root) {
        stack->root = refs->previous;
    }
    *rrefs = refs->previous;
    olcx_memory_free(refs, sizeof(OlcxReferences));
}


static void checkAndSetOldest(OlcxReferencesStack *stack, OlcxReferences ***oldest, time_t *oldestTime, OlcxReferencesStack **oldestStack) {
    if (stack->root!=NULL) {
        /* do never free the very first item, start by second WTF: Why? */
        for (OlcxReferences **refs= &(stack->root->previous); *refs!=NULL; refs= &(*refs)->previous) {
            if (*oldestTime > (*refs)->accessTime) {
                *oldestTime = (*refs)->accessTime;
                *oldest = refs;
                *oldestStack = stack;
            }
        }
    }
}

void freeOldestOlcx(void) {
    UserOlcxData          *userDataP;
    OlcxReferences        **oldest = NULL;
    OlcxReferencesStack   *oldestStack = NULL;
    time_t                oldestTime = fileProcessingStartTime;

    if (refactoringOptions.refactoringRegime != RegimeRefactory) {
        for (int i=0; i<OLCX_TAB_SIZE; i++) {
            userDataP = s_olcxTab.tab[i];
            if (userDataP!=NULL) {
                checkAndSetOldest(&userDataP->browserStack, &oldest, &oldestTime, &oldestStack);
                checkAndSetOldest(&userDataP->completionsStack, &oldest, &oldestTime, &oldestStack);
                checkAndSetOldest(&userDataP->retrieverStack, &oldest, &oldestTime, &oldestStack);
            }
        }
    }
    if (oldestTime == fileProcessingStartTime || oldest == NULL) {
        fatalError(ERR_ST, "olcxMemory memory overflow, please try again.", XREF_EXIT_ERR);
    } else {
        assert(oldest!=NULL && oldestStack!=NULL);
        deleteOlcxRefs(oldest, oldestStack);
    }
}

void olcxFreeOldCompletionItems(OlcxReferencesStack *stack) {
    OlcxReferences **references;

    references = &stack->top;
    if (*references == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
        references = &(*references)->previous;
        if (*references == NULL)
            return;
    }
    deleteOlcxRefs(references, stack);
}

void olcxInit(void) {
    void *userData[OLCX_USER_RESERVE];

    olcx_memory_init();
    s_olcxTab.tab = olcx_memory_soft_allocc(OLCX_TAB_SIZE, sizeof(UserOlcxData *));
    //CHECK_FREE(s_olcxTab.tab);        // do not report non-freeing of olcxtable
    olcxTabNoAllocInit(&s_olcxTab, OLCX_TAB_SIZE);
    /* reserve place for some users */
    for (int i=0; i<OLCX_USER_RESERVE; i++)
        userData[i] = olcx_alloc(sizeof(UserOlcxData));
    for (int i=0; i<OLCX_USER_RESERVE; i++)
        olcx_memory_free(userData[i], sizeof(UserOlcxData));
}


static void initUserOlcxData(UserOlcxData *userData, char *user) {
    userData->name = user;
    userData->browserStack.top = NULL;
    userData->browserStack.root = NULL;
    userData->completionsStack.top = NULL;
    userData->completionsStack.root = NULL;
    userData->retrieverStack.top = NULL;
    userData->retrieverStack.root = NULL;
    userData->classTree.baseClassFileIndex = noFileIndex;
    userData->classTree.tree = NULL;
    userData->next = NULL;
}


UserOlcxData *olcxSetCurrentUser(char *user) {
    UserOlcxData userData, *member;
    int size;
    char *name;

    initUserOlcxData(&userData, user);
    if (! olcxTabIsMember(&s_olcxTab, &userData, NULL, &member)) {
        // Allocate in FT memory, so it never invokes freeing of OLCX (loop?)
        FT_ALLOC(member, UserOlcxData);
        size = strlen(user)+1;
        if (size < sizeof(void*))
            size = sizeof(void*);
        FT_ALLOCC(name, size, char); // why this is in ftMem ?, some pb with free
        strcpy(name, user);
        initUserOlcxData(member, name);
        olcxTabAdd(&s_olcxTab, member);
    }
    currentUserData = member;

    return member;
}

static void olcxFreePopedStackItems(OlcxReferencesStack *stack) {
    assert(stack);
    // delete all after top
    while (stack->root != stack->top) {
        //&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->s.name);
        deleteOlcxRefs(&stack->root, stack);
    }
}

static OlcxReferences *pushOlcxReference(OlcxReferencesStack *stack) {
    OlcxReferences *res;

    res = olcx_alloc(sizeof(OlcxReferences));
    *res = (OlcxReferences){.references = NULL, .actual = NULL, .command = options.server_operation, .language = s_language,
                              .accessTime = fileProcessingStartTime, .callerPosition = noPosition, .completions = NULL, .hkSelectedSym = NULL,
                              .menuFilterLevel = DEFAULT_MENU_FILTER_LEVEL, .refsFilterLevel = DEFAULT_REFS_FILTER_LEVEL,
                              .previous = stack->top};
    return res;
}


void olcxPushEmptyStackItem(OlcxReferencesStack *stack) {
    OlcxReferences *res;
    olcxFreePopedStackItems(stack);
    res = pushOlcxReference(stack);
    stack->top = stack->root = res;
}

static bool olcxVirtualyUsageAdequate(int vApplCl, int vFunCl,
                                     int olUsage, int olApplCl, int olFunCl) {
    bool res = false;

    //&fprintf(dumpOut,"\n:checking %s\n%s\n%s\n<->\n%s\n%s\n",usageName[olUsage],fileTable.tab[olFunCl]->name,fileTable.tab[olApplCl]->name,fileTable.tab[vFunCl]->name,fileTable.tab[vApplCl]->name); fflush(dumpOut);
    if (IS_DEFINITION_OR_DECL_USAGE(olUsage)) {
        if (vFunCl == olFunCl) res = 1;
        if (isSmallerOrEqClass(olFunCl, vApplCl))
            res = true;
    } else {
        //&     if (vApplCl==vFunCl) { // only classes with definitions are considered
        if (vApplCl == olFunCl)
            res = true;
        //&         if (vFunCl == olFunCl) res = 1;
        if (isSmallerOrEqClass(vApplCl, olApplCl))
            res = true;
        //&     }
    }
    //&fprintf(dumpOut,"result is %d\n",res);fflush(dumpOut);
    return res;
}

Reference *olcxAddReferenceNoUsageCheck(Reference **rlist, Reference *ref, int bestMatchFlag) {
    Reference **place, *rr;
    rr = NULL;
    SORTED_LIST_PLACE2(place, *ref, rlist);
    if (*place==NULL || SORTED_LIST_NEQ(*place,*ref)) {
        rr = olcx_alloc(sizeof(Reference));
        *rr = *ref;
        if (LANGUAGE(LANG_JAVA)) {
            if (ref->usage.base==UsageDefined &&  bestMatchFlag) {
                rr->usage.base = UsageOLBestFitDefined;
            }
        }
        LIST_CONS(rr,(*place));
        //&fprintf(dumpOut,"olcx adding %s %s:%d:%d\n",usageName[ref->usage.base], fileTable.tab[ref->position.file]->name,ref->position.line,ref->position.col); fflush(dumpOut);
    }
    return rr;
}


Reference *olcxAddReference(Reference **rlist,Reference *ref,int bestMatchFlag) {
    Reference *rr;
    //&sprintf(tmpBuff,"checking ref %s %s:%d:%d at %d\n",usageName[ref->usage.base], simpleFileName(fileTable.tab[ref->position.file]->name),ref->position.line,ref->position.col, ref); ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
    if (! OL_VIEWABLE_REFS(ref)) return NULL; // no regular on-line refs
    rr = olcxAddReferenceNoUsageCheck(rlist, ref, bestMatchFlag);
    return rr;
}

static Reference *olcxCopyReference(Reference *ref) {
    Reference *rr;
    rr = olcx_alloc(sizeof(Reference));
    *rr = *ref;
    rr->next = NULL;
    return rr;
}

static void olcxAppendReference(Reference *ref, OlcxReferences *refs) {
    Reference *rr;
    rr = olcxCopyReference(ref);
    LIST_APPEND(Reference, refs->references, rr);
    //&fprintf(dumpOut,"olcx appending %s %s:%d:%d\n",usageName[ref->usage.base], fileTable.tab[ref->position.file]->name,ref->position.line,ref->position.col); fflush(dumpOut);
}

/* fnum is file number of which references are added, can be ANY_FILE
 */
void olcxAddReferences(Reference *list, Reference **dlist,
                       int fnum, int bestMatchFlag) {
    Reference *revlist,*tmp;
    /* from now, you should add it to macros as REVERSE_LIST_MAP() */
    revlist = NULL;
    while (list!=NULL) {
        tmp = list->next; list->next = revlist;
        revlist = list;   list = tmp;
    }
    list = revlist;
    revlist = NULL;
    while (list!=NULL) {
        if (fnum==ANY_FILE || fnum==list->position.file) {
            olcxAddReference(dlist, list, bestMatchFlag);
        }
        tmp = list->next; list->next = revlist;
        revlist = list;   list = tmp;
    }
    list = revlist;
}

void olcxAddReferenceToSymbolsMenu(SymbolsMenu  *cms, Reference *rr,
                                     int bestFitFlag) {
    Reference *added;
    added = olcxAddReference(&cms->s.refs, rr, bestFitFlag);
    if (rr->usage.base == UsageClassTreeDefinition) cms->defpos = rr->position;
    if (added!=NULL) {
        if (IS_DEFINITION_OR_DECL_USAGE(rr->usage.base)) {
            if (rr->usage.base==UsageDefined && positionsAreEqual(rr->position, cms->defpos)) {
                added->usage.base = UsageOLBestFitDefined;
            }
            if (rr->usage.base < cms->defUsage) {
                cms->defUsage = rr->usage.base;
                cms->defpos = rr->position;
            }
            cms->defRefn ++;
        } else {
            cms->refn ++;
        }
    }
}

static void olcxAddReferencesToSymbolsMenu(SymbolsMenu  *cms,
                                             Reference *rlist,
                                             int bestFitFlag
) {
    for (Reference *rr=rlist; rr!=NULL; rr=rr->next) {
        olcxAddReferenceToSymbolsMenu(cms, rr, bestFitFlag);
    }
}

void gotoOnlineCxref(Position *p, int usage, char *suffix)
{
    if (options.xref2) {
        ppcGotoPosition(p);
    } else {
        fprintf(communicationChannel,"%s%s#*+*#%d %d :%c%s ;;\n", COLCX_GOTO_REFERENCE,
                getRealFileNameStatic(fileTable.tab[p->file]->name),
                p->line, p->col, refCharCode(usage),
                suffix);
    }
}

#define IS_COMPLETION_COMMAND(comm) (                                   \
                                     (comm==OLO_COMPLETION              \
                                      || comm==OLO_SEARCH               \
                                      || comm==OLO_CGOTO                \
                                      /*&|| comm==OLO_TAG_SEARCH &*/    \
                                      )                                 \
                                     )

#define OLCX_MOVE_INIT(olcxuser,refs,checkFlag) {                       \
        assert(olcxuser);                                               \
        if (    options.server_operation==OLO_COMPLETION || options.server_operation==OLO_CSELECT \
                ||  options.server_operation==OLO_CGOTO || options.server_operation==OLO_CBROWSE \
                /*& || options.server_operation==OLO_SEARCH &*/                     \
                ||  options.server_operation==OLO_TAG_SEARCH) {                     \
            /*&refs = olcxuser->browserStack.top;&*/                    \
            /*&while (refs!=NULL && !IS_COMPLETION_COMMAND(refs->command)) refs=refs->previous;&*/ \
            refs = olcxuser->completionsStack.top;                      \
        } else {                                                        \
            refs = olcxuser->browserStack.top;                          \
            /*&while (refs!=NULL && IS_COMPLETION_COMMAND(refs->command)) refs=refs->previous;&*/ \
        }                                                               \
        if (checkFlag==CHECK_NULL && refs == NULL) {                    \
            if (options.xref2) {                                          \
                ppcBottomWarning("Empty stack");        \
            } else {                                                    \
                fprintf(communicationChannel, "=");                                    \
            }                                                           \
            return; /* tricky return from function calling this macro !!!!! */ \
        }                                                               \
    }

static void olcxRenameInit(void) {
    OlcxReferences *refs;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    refs->actual = refs->references;
    gotoOnlineCxref(&refs->actual->position, refs->actual->usage.base, "");
}


bool olcxListLessFunction(Reference *r1, Reference *r2) {
    int fc;
    char *s1,*s2;
    s1 = simpleFileName(fileTable.tab[r1->position.file]->name);
    s2 = simpleFileName(fileTable.tab[r2->position.file]->name);
    fc=strcmp(s1, s2);
    if (fc<0) return true;
    if (fc>0) return false;
    if (r1->position.file < r2->position.file) return true;
    if (r1->position.file > r2->position.file) return false;
    if (r1->position.line < r2->position.line) return true;
    if (r1->position.line > r2->position.line) return false;
    if (r1->position.col < r2->position.col) return true;
    if (r1->position.col > r2->position.col) return false;
    return false;
}

#define UsageImportantInOrder(r1,r2) (                                  \
        r1->usage.base==UsageDefined || r1->usage.base==UsageDeclared   \
        || r1->usage.base==UsageOLBestFitDefined                        \
        || r2->usage.base==UsageDefined || r2->usage.base==UsageDeclared \
        || r2->usage.base==UsageOLBestFitDefined                        \
    )

bool olcxPushLessFunction(Reference *r1, Reference *r2) {
    if (UsageImportantInOrder(r1,r2)) {
        // in definition, declaration usage is important
        if (r1->usage.base < r2->usage.base) return true;
        if (r1->usage.base > r2->usage.base) return false;
    }
    return olcxListLessFunction(r1, r2);
}


static void olcxNaturalReorder(OlcxReferences *refs) {
    LIST_MERGE_SORT(Reference, refs->references, olcxPushLessFunction);
}

static void olcxGenNoReferenceSignal(void) {
    if (options.xref2) {
        ppcBottomInformation("No reference");
    } else {
        fprintf(communicationChannel, "_");
    }
}

static void olcxOrderRefsAndGotoFirst(void) {
    OlcxReferences *refs;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    LIST_MERGE_SORT(Reference, refs->references, olcxListLessFunction);
    refs->actual = refs->references;
    if (refs->references != NULL) {
        gotoOnlineCxref(&refs->actual->position, refs->actual->usage.base, "");
    } else {
        olcxGenNoReferenceSignal();
    }
}

// references has to be ordered according internal file numbers order !!!!
static void olcxSetCurrentRefsOnCaller(OlcxReferences *refs) {
    Reference *rr;
    for (rr=refs->references; rr!=NULL; rr=rr->next){
        //&fprintf(dumpOut,"checking %d %d %d to %d %d %d\n",rr->position.file, rr->position.line,rr->position.col, refs->callerPosition.file,  refs->callerPosition.line,  refs->callerPosition.col);
        if (! positionIsLessThan(rr->position, refs->callerPosition))
            break;
    }
    // it should never be NULL, but one never knows
    if (rr == NULL) {
        refs->actual = refs->references;
    } else {
        refs->actual = rr;
    }
}

char *getJavaDocUrl_st(SymbolReferenceItem *rr) {
    static char res[MAX_REF_LEN];
    char *tt;
    int len = MAX_REF_LEN;
    res[0] = 0;
    if (rr->b.symType == TypeDefault) {
        if (rr->vFunClass==noFileIndex) {
            tt = strchr(rr->name, '.');
            if (tt==NULL) {
                sprintf(res,"packages.html#xrefproblem");
            } else {
                len = tt - rr->name;
                strncpy(res, rr->name, len);
                sprintf(res+len, ".html#");
                len = strlen(res);
                linkNamePrettyPrint(res+len, tt+1,
                                    MAX_REF_LEN-len, LONG_NAME);
            }
        } else {
            javaGetClassNameFromFileNum(rr->vFunClass, res, KEEP_SLASHES);
            for (tt=res; *tt; tt++) if (*tt=='$') *tt = '.';
            len = strlen(res);
            sprintf(res+len, ".html");
            len += strlen(res+len);
            if (strcmp(rr->name, LINK_NAME_CLASS_TREE_ITEM)!=0) {
                sprintf(res+len, "#");
                len += strlen(res+len);
                linkNamePrettyPrint(res+len, rr->name,
                                    MAX_REF_LEN-len, LONG_NAME);
            }
        }
    } else if (rr->b.symType == TypeStruct) {
        sprintf(res,"%s.html",rr->name);
    } else if (rr->b.symType == TypePackage) {
        sprintf(res,"%s/package-tree.html",rr->name);
    }
    assert(strlen(res)<MAX_REF_LEN-1);
    return res;
}

bool htmlJdkDocAvailableForUrl(char *ss){
    char        ttt[MAX_FILE_NAME_SIZE];
    char        *cp;
    int         ind;

    cp = options.htmlJdkDocAvailable;
    while (*cp!=0) {
        for (ind=0;
            cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR && cp[ind]!=':' ;
            ind++) {
            ttt[ind]=cp[ind];
            if (cp[ind]=='.')
                ttt[ind] = '/';
        }
        ttt[ind]=0;
        //&fprintf(dumpOut,"testing %s <-> %s (%d)\n", ttt, ss, ind);
        if (strncmp(ttt, ss, ind)==0)
            return true;
        cp += ind;
        if (*cp == CLASS_PATH_SEPARATOR || *cp == ':')
            cp++;
    }
    return false;
}

static bool olcxGenHtmlFileWithIndirectLink(char *ofname, char *url) {
    FILE *of;
    of = openFile(ofname, "w");
    if (of == NULL) {
        fprintf(communicationChannel,"* ** Can't open temporary file %s\n", ofname);
        return false;
    }
    fprintf(of, "<html><head>");
    if (options.urlAutoRedirect || strchr(url,' ')==NULL) {
        fprintf(of, "<META http-equiv=\"refresh\" ");
        fprintf(of, "content=\"0;URL=%s\">\n", url);
    }
    fprintf(of, "</head><body>\n");
    if (! (options.urlAutoRedirect || strchr(url,' ')==NULL)) {
        fprintf(of, "Please follow the link: ");
    }
    fprintf(of, "<A HREF=\"%s\">", url);
    fprintf(of, "%s", url);
    fprintf(of, "</A>\n");
    fprintf(of, "</body></html>");
    closeFile(of);
    return true;
}

static char *getExpandedLocalJavaDocFile_st(char *expandedPath, char *prefix, char *tmpfname) {
    static char     fullurl[11*MAX_FILE_NAME_SIZE];
    char            fullfname[MAX_FILE_NAME_SIZE];
    char            *s;
    int             cplen;

    MapOnPaths(expandedPath, {
            cplen = strlen(currentPath);
            if (cplen>0 && currentPath[cplen-1]==FILE_PATH_SEPARATOR) {
                if (prefix == NULL) {
                    sprintf(fullurl, "%s%s", currentPath, tmpfname);
                } else {
                    sprintf(fullurl, "%sapi%c%s", currentPath, FILE_PATH_SEPARATOR, tmpfname);
                }
            } else {
                if (prefix == NULL) {
                    sprintf(fullurl, "%s%c%s", currentPath, FILE_PATH_SEPARATOR, tmpfname);
                } else {
                    sprintf(fullurl, "%s%capi%c%s", currentPath, FILE_PATH_SEPARATOR, FILE_PATH_SEPARATOR, tmpfname);
                }
            }
            strcpy(fullfname, fullurl);
            if ((s=strchr(fullfname, '#'))!=NULL)
                *s = 0;
            if (fileStatus(fullfname, NULL)==0)
                return fullurl;
        });
    return NULL;
}

char *getLocalJavaDocFile_st(char *fileUrl) {
    char tmpfname[MAX_FILE_NAME_SIZE];
    static char wcJavaDocPath[MAX_OPTION_LEN];
    char *res;

    if (options.javaDocPath==NULL)
        return NULL;
    strcpy(tmpfname, fileUrl);
    for (char *ss=tmpfname; *ss; ss++)
        if (*ss == '/')
            *ss = FILE_PATH_SEPARATOR;
    expandWildcardsInPaths(options.javaDocPath, wcJavaDocPath, MAX_OPTION_LEN);
    res = getExpandedLocalJavaDocFile_st(wcJavaDocPath, NULL, tmpfname);
    // O.K. try once more time with 'api' prefixed
    if (res == NULL) {
        res = getExpandedLocalJavaDocFile_st(wcJavaDocPath, "api", tmpfname);
    }
    return res;
}

static void unBackslashifyUrl(char *url) {
#if defined (__WIN32__)
    for (char *ss=url; *ss; ss++) {
        if (*ss=='\\')
            *ss='/';
    }
#endif
}

char *getFullUrlOfJavaDoc_st(char *fileUrl) {
    static char fullUrl[MAX_CX_SYMBOL_SIZE];
    char *ss;
    ss = getLocalJavaDocFile_st(fileUrl);
    if (ss!=NULL) {
        sprintf(fullUrl,"file:///%s", ss);
    } else {
        sprintf(fullUrl, "%s/%s", options.htmlJdkDocUrl, fileUrl);
    }
    // replace backslashes under windows by slashes
    // maybe this should be on option?
    unBackslashifyUrl(fullUrl);
    return fullUrl;
}

static bool olcxBrowseSymbolInJavaDoc(SymbolReferenceItem *rr) {
    char *url, *tmd, *lfn;
    char tmpfname[MAX_FILE_NAME_SIZE];
    char theUrl[2*MAX_FILE_NAME_SIZE];
    int rrr;
    url = getJavaDocUrl_st(rr);
    lfn = getLocalJavaDocFile_st(url);
    if (lfn==NULL && (options.htmlJdkDocUrl==NULL || ! htmlJdkDocAvailableForUrl(url)))
        return false;
    if (! options.urlGenTemporaryFile) {
        if (options.xref2) {
            ppcGenRecord(PPC_BROWSE_URL, getFullUrlOfJavaDoc_st(url));
        } else {
            fprintf(communicationChannel,"~%s\n", getFullUrlOfJavaDoc_st(url));
        }
    } else {
#ifdef __WIN32__
        tmd = getenv("TEMP");
        assert(tmd);
        sprintf(tmpfname, "%s/xrefjdoc.html", tmd);
        unBackslashifyUrl(tmpfname);
#else
        tmd = getenv("LOGNAME");
        assert(tmd);
        sprintf(tmpfname, "/tmp/%sxref.html", tmd);
#endif
        rrr = olcxGenHtmlFileWithIndirectLink(tmpfname, getFullUrlOfJavaDoc_st(url));
        if (rrr) {
            sprintf(theUrl, "file:///%s", tmpfname);
            assert(strlen(theUrl)<MAX_FILE_NAME_SIZE-1);
            if (options.xref2) {
                ppcGenRecord(PPC_BROWSE_URL, theUrl);
            } else {
                fprintf(communicationChannel,"~%s\n", theUrl);
            }
        }
    }
    return true;
}

static bool checkTheJavaDocBrowsing(OlcxReferences *refs) {
    if (LANGUAGE(LANG_JAVA)) {
        for (SymbolsMenu *mm=refs->menuSym; mm!=NULL; mm=mm->next) {
            if (mm->visible && mm->selected) {
                if (olcxBrowseSymbolInJavaDoc(&mm->s))
                    return true;
            }
        }
    }
    return false;
}


static void orderRefsAndGotoDefinition(OlcxReferences *refs, int afterMenuFlag) {
    int res;
    olcxNaturalReorder(refs);
    if (refs->references == NULL) {
        refs->actual = refs->references;
        if (afterMenuFlag==PUSH_AFTER_MENU) res=0;
        else res = checkTheJavaDocBrowsing(refs);
        if (res==0) {
            olcxGenNoReferenceSignal();
        }
    } else if (refs->references->usage.base<=UsageDeclared) {
        refs->actual = refs->references;
        gotoOnlineCxref(&refs->actual->position, refs->actual->usage.base, "");
    } else {
        if (afterMenuFlag==PUSH_AFTER_MENU) res=0;
        else res = checkTheJavaDocBrowsing(refs);
        if (res==0) {
            if (options.xref2) {
                ppcWarning("Definition not found");
            } else {
                fprintf(communicationChannel,"*** Definition reference not found **");
            }
        }
    }
}

static void olcxOrderRefsAndGotoDefinition(int afterMenuFlag) {
    OlcxReferences *refs;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    orderRefsAndGotoDefinition(refs, afterMenuFlag);
}

#define GetBufChar(cch, bbb) {                                          \
        if ((bbb)->next >= (bbb)->end) {                                \
            if ((bbb)->isAtEOF || refillBuffer(bbb) == 0) {               \
                cch = EOF;                                              \
                (bbb)->isAtEOF = true;                                  \
            } else {                                                    \
                cch = * ((unsigned char*)(bbb)->next); (bbb)->next ++;  \
            }                                                           \
        } else {                                                        \
            cch = * ((unsigned char*)(bbb)->next); (bbb)->next++;       \
        }                                                               \
        /*fprintf(dumpOut,"getting char *%x < %x == '0x%x'\n",ccc,ffin,cch);fflush(dumpOut);*/ \
    }

#define GetFileChar(ch,cp,bbb) {                    \
        if (ch=='\n') {(cp)->line++; (cp)->col=0;} \
        else (cp)->col++;                          \
        GetBufChar(ch, bbb);                        \
    }

int refCharCode(int usage) {
    switch (usage) {
    case UsageOLBestFitDefined: return '!';
    case UsageDefined:  return '*';
    case UsageDeclared: return '+';
    case UsageLvalUsed: return ',';
    case UsageAddrUsed: return '.';
        /* some specials for refactorings now */
    case UsageNotFQFieldInClassOrMethod: return '.';
        // Usage Constructor definition is for move class to not expand
        // and move constructor definition references
    case UsageConstructorDefinition: return '-';
    default: return ' ';
    }
    assert(0);
}

static char s_crefListLine[MAX_REF_LIST_LINE_LEN+5];
static int s_crefListLinei = 0;

static void passSourcePutChar(int c, FILE *ff) {
    if (options.xref2) {
        if (s_crefListLinei < MAX_REF_LIST_LINE_LEN) {
            s_crefListLine[s_crefListLinei++] = c;
            s_crefListLine[s_crefListLinei] = 0;
        } else {
            strcpy(s_crefListLine + s_crefListLinei, "...");
        }
    } else {
        fputc(c,ff);
    }
}

#define LISTABLE_USAGE(rr, usages, usageFilter) (                       \
                                                 usages==USAGE_ANY || usages==rr->usage.base \
                                                     || (usages==USAGE_FILTER && rr->usage.base<usageFilter) \
                                                     )

static void linePosProcess(FILE *off,
                           int usages,
                           int usageFilter, // only if usages==USAGE_FILTER
                           char *fname,
                           Reference **rrr,
                           Position *callerp,
                           Position *cp,
                           int *cch,
                           CharacterBuffer *cxfBuf
                           ) {
    int             ch, pendingRefFlag, linerefn;
    Reference     *rr, *r;
    char            *fn;

    rr = *rrr;
    ch = *cch;
    fn = simpleFileName(getRealFileNameStatic(fname));
    //& fn = getRealFileNameStatic(fname);
    r = NULL; pendingRefFlag = 0;
    linerefn = 0;
    s_crefListLinei = 0;
    s_crefListLine[s_crefListLinei] = 0;
    do {
        if (LISTABLE_USAGE(rr, usages, usageFilter)) {
            if (r==NULL || r->usage.base > rr->usage.base)
                r = rr;
            if (pendingRefFlag) {
                if (! options.xref2) fprintf(off,"\n");
            }
            if (options.xref2) {
                if (! pendingRefFlag) {
                    sprintf(s_crefListLine+s_crefListLinei, "%s:%d:", fn, rr->position.line);
                    s_crefListLinei += strlen(s_crefListLine+s_crefListLinei);
                }
            } else {
                if (positionsAreNotEqual(*callerp, rr->position)) fprintf(off, " ");
                else fprintf(off, ">");
                fprintf(off,"%c%s:%d:",refCharCode(rr->usage.base),fn,
                        rr->position.line);
            }
            linerefn++;
            pendingRefFlag = 1;
        }
        rr=rr->next;
    } while (rr!=NULL && ((rr->position.file == cp->file && rr->position.line == cp->line)
                          || (rr->usage.base>UsageMaxOLUsages)));
    if (r!=NULL) {
        if (! cxfBuf->isAtEOF) {
            while (ch!='\n' && (! cxfBuf->isAtEOF)) {
                passSourcePutChar(ch,off);
                GetFileChar(ch, cp, cxfBuf);
            }
        }
        if (! options.xref2) passSourcePutChar('\n',off);
    }
    if (options.xref2 && s_crefListLinei!=0) {
        ppcIndent();
        fprintf(off, "<%s %s=%d %s=%ld>%s</%s>\n",
                PPC_SRC_LINE, PPCA_REFN, linerefn,
                PPCA_LEN, (unsigned long)strlen(s_crefListLine),
                s_crefListLine, PPC_SRC_LINE);
    }
    *rrr = rr;
    *cch = ch;
}

static Reference *passNonPrintableRefsForFile(Reference *references,
                                              int wantedFileNumber,
                                              int usages, int usageFilter) {
    Reference *r;

    for (r=references; r!=NULL && r->position.file == wantedFileNumber; r=r->next) {
        if (LISTABLE_USAGE(r, usages, usageFilter))
            return r;
    }
    return r;
}

static void passRefsThroughSourceFile(Reference **in_out_references, Position *callerp,
                                      FILE *outputFile, int usages, int usageFilter) {
    Reference *references,*oldrr;
    int ch,fnum;
    EditorBuffer *ebuf;
    char *cofileName;
    Position cp;
    CharacterBuffer cxfBuf;

    references = *in_out_references;
    if (references==NULL)
        goto fin;
    fnum = references->position.file;
    assert(fileTable.tab[fnum]);
    cofileName = fileTable.tab[fnum]->name;
    references = passNonPrintableRefsForFile(references, fnum, usages, usageFilter);
    if (references==NULL || references->position.file != fnum)
        goto fin;
    if (options.referenceListWithoutSource) {
        ebuf = NULL;
    } else {
        ebuf = editorFindFile(cofileName);
        if (ebuf==NULL) {
            if (options.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "file '%s' not accessible", cofileName);
                errorMessage(ERR_ST, tmpBuff);
            } else {
                fprintf(outputFile,"!!! file '%s' is not accessible", cofileName);
            }
        }
    }
    ch = ' ';
    if (ebuf==NULL) {
        cxfBuf.isAtEOF = true;
    } else {
        fillCharacterBuffer(&cxfBuf, ebuf->allocation.text, ebuf->allocation.text+ebuf->allocation.bufferSize, NULL, ebuf->allocation.bufferSize, noFileIndex, ebuf->allocation.text);
        GetFileChar(ch, &cp, &cxfBuf);
    }
    cp = makePosition(references->position.file, 1, 0);
    oldrr=NULL;
    while (references!=NULL && references->position.file==cp.file && references->position.line>=cp.line) {
        assert(oldrr!=references); oldrr=references;    // because it is a dangerous loop
        while ((! cxfBuf.isAtEOF) && cp.line<references->position.line) {
            while (ch!='\n' && ch!=EOF)
                GetBufChar(ch, &cxfBuf);
            GetFileChar(ch, &cp, &cxfBuf);
        }
        linePosProcess(outputFile, usages, usageFilter, cofileName,
                       &references, callerp, &cp, &ch, &cxfBuf);
    }
    //&if (cofile != NULL) closeFile(cofile);
 fin:
    *in_out_references = references;
}

/* ******************************************************************** */

#if 0
static void olcxDumpSelectionMenu(SymbolsMenu *menu) {
    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        xxxxfprintf(dumpOut,">> %d/%d %s %s %s %d\n", ss->defRefn, ss->refn, ss->s.name, simpleFileName(fileTable.tab[ss->s.vFunClass]->name), simpleFileName(fileTable.tab[ss->s.vApplClass]->name), ss->outOnLine);
    }
    fprintf(dumpOut,"\n\n");
}
#endif

bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2) {
    if ((oo1&OOC_PROFILE_MASK) < (oo2&OOC_PROFILE_MASK)) {
        return false;
    }
    if ((oo1&OOC_VIRTUAL_MASK) < (oo2&OOC_VIRTUAL_MASK)) {
        return false;
    }
    return true;
}

void olcxPrintClassTree(SymbolsMenu *sss) {
    if (options.xref2) {
        ppcBegin(PPC_DISPLAY_CLASS_TREE);
    } else {
        fprintf(communicationChannel, "<");
    }
    readOneAppropReferenceFile(NULL, classHierarchyFunctionSequence);
    generateGlobalReferenceLists(sss, communicationChannel, "__NO_HTML_FILE_NAME!__");
    if (options.xref2) ppcEnd(PPC_DISPLAY_CLASS_TREE);
}

void olcxPrintSelectionMenu(SymbolsMenu *sss) {
    if (options.xref2) {
        ppcBegin(PPC_SYMBOL_RESOLUTION);
    } else {
        fprintf(communicationChannel, ">");
    }
    if (sss!=NULL) {
        readOneAppropReferenceFile(NULL, classHierarchyFunctionSequence);
        generateGlobalReferenceLists(sss, communicationChannel, "__NO_HTML_FILE_NAME!__");
    }
    if (options.xref2) {
        ppcEnd(PPC_SYMBOL_RESOLUTION);
    } else {
        if (options.server_operation==OLO_RENAME || options.server_operation==OLO_ARG_MANIP || options.server_operation==OLO_ENCAPSULATE) {
            if (LANGUAGE(LANG_JAVA)) {
                fprintf(communicationChannel, "-![Warning] It is highly recommended to process the whole hierarchy of related classes at once. Unselection of any class of applications above (and its exclusion from refactoring process) may cause changes in your program behavior. Press <return> to continue.\n");
            } else {
                fprintf(communicationChannel, "-![Warning] It is highly recommended to process all symbols at once. Unselection of any symbols and its exclusion from refactoring process may cause changes in your program behavior. Press <return> to continue.\n");
            }
        }
        if (options.server_operation==OLO_VIRTUAL2STATIC_PUSH) {
            fprintf(communicationChannel, "-![Warning] If you see this message it is highly probable that turning this virtual method into static will not be behaviour preserving! This refactoring is behaviour preserving only if the method does not use mechanism of virtual invocations. On this screen you should select the application classes which are refering to the method which will become static. If you can't unambiguously determine those references do not continue in this refactoring!\n");
        }
        if (options.server_operation==OLO_SAFETY_CHECK2) {
            if (LANGUAGE(LANG_JAVA)) {
                fprintf(communicationChannel, "-![Warning] There are differences between original class hierarchy and the new one, those name clashes may cause that the refactoring will not be behavior preserving!\n");
            } else {
                fprintf(communicationChannel, "-![Error] There is differences between original and new symbols referenced at this position. The difference is due to name clashes and may cause changes in the behaviour of the program. Please, undo last refactoring!");
            }
        }
        if (options.server_operation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK) {
            fprintf(communicationChannel, "-![Warning] A method (getter or setter) created during the encapsulation has the same name as an existing method, so it will be inserted into this (existing) inheritance hierarchy. This may cause that the refactoring will not be behaviour preserving. Please, select applications unambiguously reporting to the newly created method. If you can't do this, you should undo the refactoring and rename the field first!\n");
        }
    }
}

static int getCurrentRefPosition(OlcxReferences *refs) {
    Reference     *rr;
    int             rlevel;
    int             actn = 0;
    rr = NULL;
    if (refs!=NULL) {
        rlevel = s_refListFilters[refs->refsFilterLevel];
        for (rr=refs->references; rr!=NULL && rr!=refs->actual; rr=rr->next) {
            if (rr->usage.base < rlevel)
        actn++;
        }
    }
    if (rr==NULL)
        actn = 0;
    return actn;
}

static void symbolHighlighNameSprint(char *output, SymbolsMenu *ss) {
    char *bb, *cc;
    int len, llen;
    sprintfSymbolLinkName(output, ss);
    cc = strchr(output, '(');
    if (cc != NULL) *cc = 0;
    len = strlen(output);
    output[len]=0;
    bb = lastOccurenceInString(output,'.');
    if (bb!=NULL) {
        bb++;
        llen = strlen(bb);
        memmove(output, bb, llen+1);
        output[llen]=0;
    }
}

static void olcxPrintRefList(char *commandString, OlcxReferences *refs) {
    Reference *rr;
    int         actn, len;
    char        ttt[MAX_CX_SYMBOL_SIZE];

    if (options.xref2) {
        actn = getCurrentRefPosition(refs);
        if (refs!=NULL && refs->menuSym != NULL) {
            ttt[0]='\"';
            symbolHighlighNameSprint(ttt+1, refs->menuSym);
            len = strlen(ttt);
            ttt[len]='\"';
            ttt[len+1]=0;
            ppcBeginWithNumericValueAndAttribute(PPC_REFERENCE_LIST, actn,
                                            PPCA_SYMBOL, ttt);
        } else {
            ppcBeginWithNumericValue(PPC_REFERENCE_LIST, actn);
        }
    } else {
        fprintf(communicationChannel,"%s",commandString);/* communication char */
    }
    if (refs!=NULL) {
        rr=refs->references;
        while (rr != NULL) {
            passRefsThroughSourceFile(&rr, &refs->actual->position,
                                      communicationChannel, USAGE_FILTER,
                                      s_refListFilters[refs->refsFilterLevel]);
        }
    }
    if (options.xref2) {
        ppcEnd(PPC_REFERENCE_LIST);
        //& if (refs!=NULL && refs->actual!=NULL) ppcGotoPosition(&refs->actual->position);
    }
    fflush(communicationChannel);
}

static void olcxReferenceList(char *commandString) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs, CHECK_NULL);
    olcxPrintRefList(commandString, refs);
}

static void olcxListTopReferences(char *commandString) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    olcxPrintRefList(commandString, refs);
}

static void olcxGenGotoActReference(OlcxReferences *refs) {
    if (refs->actual != NULL) {
        gotoOnlineCxref(&refs->actual->position, refs->actual->usage.base, "");
    } else {
        olcxGenNoReferenceSignal();
    }
}

static void olcxPushOnly(void) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData, refs, CHECK_NULL);
    //&LIST_MERGE_SORT(Reference, refs->references, olcxListLessFunction);
    olcxGenGotoActReference(refs);
}

static void olcxPushAndCallMacro(void) {
    OlcxReferences    *refs;
    char                symbol[MAX_CX_SYMBOL_SIZE];

    OLCX_MOVE_INIT(currentUserData, refs, CHECK_NULL);
    LIST_MERGE_SORT(Reference, refs->references, olcxListLessFunction);
    LIST_REVERSE(Reference, refs->references);
    assert(options.xref2);
    symbolHighlighNameSprint(symbol, refs->hkSelectedSym);
    // precheck first
    for (Reference *rr=refs->references; rr!=NULL; rr=rr->next) {
        ppcReferencePreCheck(rr, symbol);
    }
    for (Reference *rr=refs->references; rr!=NULL; rr=rr->next) {
        ppcReferencePreCheck(rr, symbol);
        ppcGenRecord(PPC_CALL_MACRO, "");
    }
    LIST_REVERSE(Reference, refs->references);
}

static void olcxReferenceGotoRef(int refn) {
    OlcxReferences    *refs;
    Reference         *rr;
    int                 i,rfilter;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    rfilter = s_refListFilters[refs->refsFilterLevel];
    for (rr=refs->references,i=1; rr!=NULL && (i<refn||rr->usage.base>=rfilter); rr=rr->next){
        if (rr->usage.base < rfilter) i++;
    }
    refs->actual = rr;
    olcxGenGotoActReference(refs);
}

static S_olCompletion *olCompletionNthLineRef(S_olCompletion *cpls, int refn) {
    S_olCompletion *rr, *rrr;
    int i;

    for (rr=rrr=cpls, i=1; i<=refn && rrr!=NULL; rrr=rrr->next) {
        i += rrr->lineCount;
        rr = rrr;
    }
    return rr;
}

static void olcxPopUser(void) {
    currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
}

static void olcxPopUserAndFreePoped(void) {
    olcxPopUser();
    olcxFreePopedStackItems(&currentUserData->browserStack);
}

static OlcxReferences *olcxPushUserOnPhysicalTopOfStack(void) {
    OlcxReferences *oldtop;
    oldtop = currentUserData->browserStack.top;
    currentUserData->browserStack.top = currentUserData->browserStack.root;
    olcxPushEmptyStackItem(&currentUserData->browserStack);
    return oldtop;
}

static void olcxPopAndFreeAndPopsUntil(OlcxReferences *oldtop) {
    olcxPopUserAndFreePoped();
    // recover old top, but what if it was freed, hmm
    while (currentUserData->browserStack.top!=NULL && currentUserData->browserStack.top!=oldtop) {
        olcxPopUser();
    }
}

static void olcxFindDefinitionAndGenGoto(SymbolReferenceItem *sym) {
    OlcxReferences *refs, *oldtop;
    SymbolsMenu mmm;

    // preserve poped items from browser first
    oldtop = olcxPushUserOnPhysicalTopOfStack();
    refs = currentUserData->browserStack.top;
    fillSymbolsMenu(&mmm, *sym, 1,1,0,UsageUsed,0,0,0,UsageNone,noPosition,0, NULL, NULL);
    //&oldrefs = *refs;
    refs->menuSym = &mmm;
    readOneAppropReferenceFile(sym->name, fullScanFunctionSequence);
    orderRefsAndGotoDefinition(refs, DEFAULT_VALUE);
    //&olcxFreeReferences(refs->references);
    //&*refs = oldrefs;
    refs->menuSym = NULL;
    // recover stack
    olcxPopAndFreeAndPopsUntil(oldtop);
}

static void olcxReferenceGotoCompletion(int refn) {
    OlcxReferences *refs;
    S_olCompletion *rr;

    assert(refn > 0);
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    rr = olCompletionNthLineRef(refs->completions, refn);
    if (rr != NULL) {
        if (rr->category == CategoryLocal /*& || refs->command == OLO_TAG_SEARCH &*/) {
            if (rr->ref.usage.base != UsageClassFileDefinition
                && rr->ref.usage.base != UsageClassTreeDefinition
                && positionsAreNotEqual(rr->ref.position, noPosition)) {
                gotoOnlineCxref(&rr->ref.position, UsageDefined, "");
            } else {
                if (!olcxBrowseSymbolInJavaDoc(&rr->sym))
                    olcxGenNoReferenceSignal();
            }
        } else {
            olcxFindDefinitionAndGenGoto(&rr->sym);
        }
    } else {
        olcxGenNoReferenceSignal();
    }
}

static void olcxReferenceGotoTagSearchItem(int refn) {
    S_olCompletion      *rr;

    assert(refn > 0);
    assert(currentUserData);
    assert(currentUserData->retrieverStack.top);
    rr = olCompletionNthLineRef(currentUserData->retrieverStack.top->completions, refn);
    if (rr != NULL) {
        if (rr->ref.usage.base != UsageClassFileDefinition
            && rr->ref.usage.base != UsageClassTreeDefinition
            && positionsAreNotEqual(rr->ref.position, noPosition)) {
            gotoOnlineCxref(&rr->ref.position, UsageDefined, "");
        } else {
            if (!olcxBrowseSymbolInJavaDoc(&rr->sym))
                olcxGenNoReferenceSignal();
        }
    } else {
        olcxGenNoReferenceSignal();
    }
}

static void olcxReferenceBrowseCompletion(int refn) {
    OlcxReferences    *refs;
    S_olCompletion      *rr;
    char                *url;

    assert(refn > 0);
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    rr = olCompletionNthLineRef(refs->completions, refn);
    if (rr != NULL) {
        if (rr->category == CategoryLocal) {
            if (options.xref2)
                ppcGenRecord(PPC_ERROR, "No JavaDoc is available for local symbols.");
            else
                fprintf(communicationChannel,"* ** no JavaDoc is available for local symbols **");
        } else {
            if (!olcxBrowseSymbolInJavaDoc(&rr->sym)) {
                char message[TMP_BUFF_SIZE];
                int len;

                sprintf(message, "*** JavaDoc for ");
                url = getJavaDocUrl_st(&rr->sym);
                len = strlen(message);
                for (char *tt=url ; *tt && *tt!='#'; tt++, len++)
                    message[len] = *tt;
                message[len] = '\0';
                strcat(message, " not available, (check -javadocpath) **");
                if (options.xref2)
                    ppcGenRecord(PPC_ERROR, message);
                else
                    fprintf(communicationChannel, "%s", message);
            }
        }
    } else {
        if (options.xref2)
            ppcGenRecord(PPC_ERROR, "Out of range");
        else
            fprintf(communicationChannel, "* ** out of range **");
    }
}

static void olcxSetActReferenceToFirstVisible(OlcxReferences *refs, Reference *r) {
    int                 rlevel;
    rlevel = s_refListFilters[refs->refsFilterLevel];
    while (r!=NULL && r->usage.base>=rlevel) r = r->next;
    if (r != NULL) {
        refs->actual = r;
    } else {
        if (options.xref2) {
            ppcBottomInformation("Moving to the first reference");
        }
        r = refs->references;
        while (r!=NULL && r->usage.base>=rlevel) r = r->next;
        refs->actual = r;
    }
}

static void olcxReferencePlus(void) {
    OlcxReferences    *refs;
    Reference         *r;
    OLCX_MOVE_INIT(currentUserData, refs, CHECK_NULL);
    if (refs->actual == NULL)
        refs->actual = refs->references;
    else {
        r = refs->actual->next;
        olcxSetActReferenceToFirstVisible(refs, r);
    }
    olcxGenGotoActReference(refs);
}

static void olcxReferenceMinus(void) {
    OlcxReferences    *refs;
    Reference         *r,*l,*act;
    int                 rlevel;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    rlevel = s_refListFilters[refs->refsFilterLevel];
    if (refs->actual == NULL) refs->actual = refs->references;
    else {
        act = refs->actual;
        l = NULL;
        for (r=refs->references; r!=act && r!=NULL; r=r->next) {
            if (r->usage.base < rlevel)
                l = r;
        }
        if (l==NULL) {
            if (options.xref2) {
                ppcBottomInformation("Moving to the last reference");
            }
            for (; r!=NULL; r=r->next) {
                if (r->usage.base < rlevel)
                    l = r;
            }
        }
        refs->actual = l;
    }
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoDef(void) {
    OlcxReferences    *refs;
    Reference         *dr;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    dr = getDefinitionRef(refs->references);
    if (dr != NULL) refs->actual = dr;
    else refs->actual = refs->references;
    //&fprintf(dumpOut,"goto ref %d %d\n", refs->actual->position.line, refs->actual->position.col);
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoCurrent(void) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGetCurrentRefn(void) {
    OlcxReferences    *refs;
    int                 n;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    n = getCurrentRefPosition(refs);
    assert(options.xref2);
    ppcValueRecord(PPC_UPDATE_CURRENT_REFERENCE, n, "");
}

static void olcxReferenceGotoCaller(void) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    if (refs->callerPosition.file != noFileIndex) {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, "");

    } else {
        olcxGenNoReferenceSignal();
    }
}

#define MAX_SYMBOL_MESSAGE_LEN 50

static void olcxPrintSymbolName(OlcxReferences *refs) {
    char ttt[MAX_CX_SYMBOL_SIZE+MAX_SYMBOL_MESSAGE_LEN];
    SymbolsMenu *ss;
    if (refs==NULL) {
        if (options.xref2) {
            ppcBottomInformation("stack is now empty");
        } else {
            fprintf(communicationChannel, "*stack is now empty");
        }
        //&     fprintf(communicationChannel, "*");
    } else if (refs->hkSelectedSym==NULL) {
        if (options.xref2) {
            ppcBottomInformation("Current top symbol: <empty>");
        } else {
            fprintf(communicationChannel, "*Current top symbol: <empty>");
        }
    } else {
        ss = refs->hkSelectedSym;
        if (options.xref2) {
            sprintf(ttt, "Current top symbol: ");
            assert(strlen(ttt) < MAX_SYMBOL_MESSAGE_LEN);
            sprintfSymbolLinkName(ttt+strlen(ttt), ss);
            ppcBottomInformation(ttt);
        } else {
            fprintf(communicationChannel, "*Current top symbol: ");
            printSymbolLinkName(communicationChannel, ss);
        }
    }
}


static void olcxShowTopSymbol(void) {
    OlcxReferences    *refs;

    OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    //& OLCX_MOVE_INIT(currentUserData,refs, CHECK_NULL);
    olcxPrintSymbolName(refs);
}

static int referenceLess(Reference *r1, Reference *r2) {
    return positionIsLessThan(r1->position, r2->position);
}

static SymbolsMenu *findSymbolCorrespondingToReference(SymbolsMenu *menu,
                                                       Reference *ref
) {
    Reference *rr;

    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        SORTED_LIST_FIND3(rr, Reference, ref, ss->s.refs, referenceLess);
        if (rr!=NULL && positionsAreEqual(rr->position, ref->position)) {
            return ss;
        }
    }
    return NULL;
}

static void olcxShowTopApplClass(void) {
    OlcxReferences    *refs;
    SymbolsMenu     *mms;
    //& OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    OLCX_MOVE_INIT(currentUserData,refs, CHECK_NULL);
    assert(refs->actual!=NULL);
    mms = findSymbolCorrespondingToReference(refs->menuSym, refs->actual);
    if (mms==NULL) {
        olcxGenNoReferenceSignal();
    } else {
        fprintf(communicationChannel, "*");
        printClassFqtNameFromClassNum(communicationChannel, mms->s.vApplClass);
    }
}

static void olcxShowTopType(void) {
    OlcxReferences    *refs;
    SymbolsMenu     *mms;
    //& OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    OLCX_MOVE_INIT(currentUserData,refs, CHECK_NULL);
    assert(refs->actual!=NULL);
    mms = findSymbolCorrespondingToReference(refs->menuSym, refs->actual);
    if (mms==NULL) {
        olcxGenNoReferenceSignal();
    } else {
        fprintf(communicationChannel, "*%s",typeNamesTable[mms->s.b.symType]);
    }
}

static void olcxShowClassTree(void) {
    olcxPrintClassTree(currentUserData->classTree.tree);
}

SymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi,int storage){
    SymbolsMenu     *res;
    SymbolReferenceItem     ss;

    fillSymbolRefItem(&ss, fieldName, cxFileHashNumber(fieldName),
                                cfi, cfi);
    fillSymbolRefItemBits(&ss.b, TypeDefault, storage, ScopeGlobal,
                           AccessDefault, CategoryGlobal);
    res = olCreateNewMenuItem(&ss, ss.vApplClass, ss.vFunClass, &noPosition, UsageNone,
                              1, 1, OOC_VIRT_SAME_APPL_FUN_CLASS,
                              UsageUsed, 0);
    return res;
}

static bool refItemsOrderLess(SymbolsMenu *ss1, SymbolsMenu *ss2) {
    SymbolReferenceItem *s1, *s2;
    int r;
    char *n1, *n2;
    int len1;
    int len2;
    UNUSED len1;
    UNUSED len2;

    s1 = &ss1->s; s2 = &ss2->s;
    GET_BARE_NAME(s1->name, n1, len1);
    GET_BARE_NAME(s2->name, n2, len2);
    r = strcmp(n1, n2);
    if (r!=0)
        return r<0;
    r = strcmp(s1->name, s2->name);
    if (r!=0)
        return r<0;
    return classHierarchyClassNameLess(ss1->s.vApplClass,ss2->s.vApplClass);
}

static void olcxTopSymbolResolution(void) {
    OlcxReferences    *refs;
    SymbolsMenu     *ss;
    OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    ss = NULL;
    if (refs!=NULL) {
        LIST_MERGE_SORT(SymbolsMenu,
                        refs->menuSym,
                        refItemsOrderLess);
        ss = refs->menuSym;
    }
    olcxPrintSelectionMenu(ss);
}

/* NOTE: may return! */
#define CHECK_ATTRIBUTES(p1,p2) {                                       \
        if (p1 == p2) return true;                                      \
        if (p1->b.category != p2->b.category) return false;             \
        if (p1->b.symType!=TypeCppCollate && p2->b.symType!=TypeCppCollate && p1->b.symType!=p2->b.symType) return false; \
        if (p1->b.storage!=p2->b.storage) return false;                 \
    }

bool isSameCxSymbol(SymbolReferenceItem *p1, SymbolReferenceItem *p2) {
    /* Expansion of CHECK_ATTRIBUTES() macro to be explicit about returns... */
    if (p1 == p2)
        return true;
    if (p1->b.category != p2->b.category)
        return false;
    if (p1->b.symType!=TypeCppCollate && p2->b.symType!=TypeCppCollate && p1->b.symType!=p2->b.symType)
        return false;
    if (p1->b.storage!=p2->b.storage)
        return false;

    if (strcmp(p1->name, p2->name) != 0)
        return false;
    return true;
}

bool isSameCxSymbolIncludingFunctionClass(SymbolReferenceItem *p1, SymbolReferenceItem *p2) {
    if (p1->vFunClass != p2->vFunClass)
        return false;
    return isSameCxSymbol(p1, p2);
}

bool isSameCxSymbolIncludingApplicationClass(SymbolReferenceItem *p1, SymbolReferenceItem *p2) {
    if (p1->vApplClass != p2->vApplClass)
        return false;
    return isSameCxSymbol(p1, p2);
}

bool olcxIsSameCxSymbol(SymbolReferenceItem *p1, SymbolReferenceItem *p2) {
    int n1len, n2len;
    char *n1start, *n2start;

    //& CHECK_ATTRIBUTES(p1,p2);  // to show panic macro and panic symbol and to make safetycheck working
    GET_BARE_NAME(p1->name, n1start, n1len);
    GET_BARE_NAME(p2->name, n2start, n2len);
    if (n1len != n2len)
        return false;
    if (strncmp(n1start, n2start, n1len))
        return false;
    return true;
}

void olStackDeleteSymbol(OlcxReferences *refs) {
    OlcxReferences **rr;
    for (rr= &currentUserData->browserStack.root; *rr!=NULL&&*rr!=refs; rr= &(*rr)->previous)
        ;
    assert(*rr != NULL);
    deleteOlcxRefs(rr, &currentUserData->browserStack);
}

static void olcxGenInspectClassDefinitionRef(int classnum) {
    SymbolReferenceItem mmm;
    char ccc[MAX_CX_SYMBOL_SIZE];

    javaGetClassNameFromFileNum(classnum, ccc, KEEP_SLASHES);
    fillSymbolRefItem(&mmm, ccc, cxFileHashNumber(ccc),
                      noFileIndex, noFileIndex);
    fillSymbolRefItemBits(&mmm.b, TypeStruct, StorageExtern, ScopeGlobal,
                          AccessDefault, CategoryGlobal);
    olcxFindDefinitionAndGenGoto(&mmm);
}

static void olcxMenuInspectDef(SymbolsMenu *menu, int inspect) {
    SymbolsMenu *ss;

    for (ss=menu; ss!=NULL; ss=ss->next) {
        //&sprintf(tmpBuff,"checking line %d", ss->outOnLine); ppcBottomInformation(tmpBuff);
        int line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
        if (line == options.olcxMenuSelectLineNum)
            goto breakl;
    }
 breakl:
    if (ss == NULL) {
        olcxGenNoReferenceSignal();
    } else {
        if (inspect == INSPECT_DEF) {
            if (ss->defpos.file>=0 && ss->defpos.file!=noFileIndex) {
                gotoOnlineCxref(&ss->defpos, UsageDefined, "");
            } else if (!olcxBrowseSymbolInJavaDoc(&ss->s)) {
                olcxGenNoReferenceSignal();
            }
        } else {
            // inspect class
            olcxGenInspectClassDefinitionRef(ss->s.vApplClass);
        }
    }
}

static void olcxSymbolMenuInspectClass(void) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    olcxMenuInspectDef(refs->menuSym, INSPECT_CLASS);
}

static void olcxSymbolMenuInspectDef(void) {
    OlcxReferences    *refs;
    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    olcxMenuInspectDef(refs->menuSym, INSPECT_DEF);
}

static void olcxClassTreeInspectDef(void) {
    assert(currentUserData);
    olcxMenuInspectDef(currentUserData->classTree.tree, INSPECT_CLASS);
}

void olProcessSelectedReferences(
                                 OlcxReferences    *rstack,
                                 void (*referencesMapFun)(OlcxReferences *rstack, SymbolsMenu *ss)
                                 ) {
    if (rstack->menuSym == NULL)
        return;

    LIST_MERGE_SORT(Reference, rstack->references, olcxReferenceInternalLessFunction);
    for (SymbolsMenu *ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
        //&LIST_LEN(nn, Reference, ss->s.refs);sprintf(tmpBuff,"xxx1 %d refs for %s", nn, fileTable.tab[ss->s.vApplClass]->name);ppcBottomInformation(tmpBuff);
        referencesMapFun(rstack, ss);
    }
    olcxSetCurrentRefsOnCaller(rstack);
    LIST_MERGE_SORT(Reference, rstack->references, olcxListLessFunction);
}

void olcxRecomputeSelRefs(OlcxReferences *refs) {
    // [28/12] putting it into comment, just to see, if it works
    // otherwise it clears stack on pop on empty stack
    //& olcxFreePopedStackItems(&currentUserData->browserStack);
    olcxFreeReferences(refs->references); refs->references = NULL;
    olProcessSelectedReferences(refs, genOnLineReferences);
}

static void olcxMenuToggleSelect(void) {
    OlcxReferences    *refs;
    SymbolsMenu     *ss;
    int                 line;
    char                ln[MAX_REF_LEN];
    char                *cname;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    for (ss=refs->menuSym; ss!=NULL; ss=ss->next) {
        line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
        if (line == options.olcxMenuSelectLineNum) {
            ss->selected = ss->selected ^ 1;
            olcxRecomputeSelRefs(refs);
            break;
        }
    }
    if (options.xref2) {
        if (ss!=NULL) {
            olcxPrintRefList(";", refs);
        }
    } else {
        if (ss==NULL) {
            olcxGenNoReferenceSignal();
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            linkNamePrettyPrint(ln, ss->s.name,MAX_REF_LEN,SHORT_NAME);
            cname = javaGetNudePreTypeName_st(getRealFileNameStatic(
                                                                    fileTable.tab[ss->s.vApplClass]->name),
                                              options.nestedClassDisplaying);
            sprintf(tmpBuff, "%s %s refs of \"%s\"",
                    (ss->selected?"inserting":"removing"), cname, ln);
            fprintf(communicationChannel,"*%s", tmpBuff);
            //&fprintf(communicationChannel,"\"%s\" refs of \"%s\"", fileTable.tab[ss->s.vApplClass]->name, ss->s.name);
        }
    }
}

static void olcxMenuSelectOnly(void) {
    OlcxReferences *refs;
    SymbolsMenu *sel;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    sel = NULL;
    for (SymbolsMenu *ss=refs->menuSym; ss!=NULL; ss=ss->next) {
        ss->selected = 0;
        int line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
        if (line == options.olcxMenuSelectLineNum) {
            ss->selected = 1;
            sel = ss;
        }
    }
    if (sel==NULL) {
        if (options.xref2) {
            ppcBottomWarning("No Symbol");
        } else {
            fprintf(communicationChannel,"*No symbol");
        }
        return;
    }
    olcxRecomputeSelRefs(refs);
    if (options.xref2) {
        Reference *dref = getDefinitionRef(refs->references);
        if (dref != NULL) refs->actual = dref;
        olcxPrintRefList(";", refs);
        if (dref == NULL) {
            if (sel!=NULL && sel->s.vApplClass!=sel->s.vFunClass) {
                char ttt[MAX_CX_SYMBOL_SIZE];
                char tmpBuff[TMP_BUFF_SIZE];
                sprintfSymbolLinkName(ttt, sel);
                sprintf(tmpBuff,"Class %s does not define %s", javaGetShortClassNameFromFileNum_st(sel->s.vApplClass), ttt);
                ppcBottomWarning(tmpBuff);
            } else {
                if (!olcxBrowseSymbolInJavaDoc(&sel->s)) {  //& checkTheJavaDocBrowsing(refs);
                    ppcBottomWarning("Definition not found");
                } else {
                    ppcBottomInformation("Definition not found, loading javadoc.");
                }
            }
        } else {
            ppcGotoPosition(&refs->actual->position);
        }
    } else {
        fprintf(communicationChannel, "*");
    }
}


static void selectUnusedSymbols(SymbolsMenu *mm, void *vflp, void *p2) {
    bool atleastOneSelected;
    int filter, *flp;

    flp = (int *)vflp;
    filter = *flp;
    for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
        ss->visible = 1; ss->selected = 0;
    }
    if (mm->s.b.storage != StorageField && mm->s.b.storage != StorageMethod) {
        for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
            if (ss->defRefn!=0 && ss->refn==0) ss->selected = 1;
        }
        goto fini;
    }
    atleastOneSelected = true;
    while (atleastOneSelected) {
        atleastOneSelected = false;
        // O.K. find definition
        for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
            if (ss->selected==0 && ss->defRefn!=0) {
                assert(ss->s.vFunClass == ss->s.vApplClass);
                // O.K. is it potentially used?
                bool used = false;
                for (SymbolsMenu *s=mm; s!=NULL; s=s->next) {
                    if (ss->s.vFunClass == s->s.vFunClass) {
                        if (s->refn != 0) {
                            used = true;
                            goto checked;
                        }
                    }
                }
                // for method, check if can be used in a superclass
                if (ss->s.b.storage == StorageMethod) {
                    for (SymbolsMenu *s=mm; s!=NULL; s=s->next) {
                        if (s->s.vFunClass == s->s.vApplClass       // it is a definition
                            && ss->s.vApplClass != s->s.vApplClass  // not this one
                            && s->selected == 0                     // used
                            && isSmallerOrEqClass(ss->s.vApplClass, s->s.vApplClass)) {
                            used = true;
                            goto checked;
                        }
                    }
                }
            checked:
                if (!used) {
                    atleastOneSelected = true;
                    ss->selected = 1;
                }
            }
        }
    }
 fini:
    for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
        if (ss->selected) goto fini2;
    }
    //nothing selected, make the symbol unvisible
    for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
        ss->visible = 0;
    }
 fini2:
    if (filter>0) {
        // make all unselected unvisible
        for (SymbolsMenu *ss=mm; ss!=NULL; ss=ss->next) {
            if (ss->selected == 0) ss->visible = 0;
        }
    }
    return;
}


static void olcxMenuSelectAll(int val) {
    OlcxReferences *refs;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    if (refs->command == OLO_GLOBAL_UNUSED) {
        if (options.xref2) {
            ppcGenRecord(PPC_WARNING, "The browser does not display project unused symbols anymore");
        }
    }
    for (SymbolsMenu *ss=refs->menuSym; ss!=NULL; ss=ss->next) {
        if (ss->visible) ss->selected = val;
    }
    olcxRecomputeSelRefs(refs);
    if (options.xref2) {
        olcxPrintRefList(";", refs);
    } else {
        fprintf(communicationChannel, "*Done");
    }
}

static void setDefaultSelectedVisibleItems(SymbolsMenu *menu,
                                           unsigned ooVisible,
                                           unsigned ooSelected
) {
    bool select, visible;
    unsigned ooBits;

    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        ooBits = ss->ooBits;
        visible = ooBitsGreaterOrEqual(ooBits, ooVisible);
        select = false;
        if (visible) {
            select=ooBitsGreaterOrEqual(ooBits, ooSelected);
            if (ss->s.b.symType==TypeCppCollate)
                select=false;
        }
        ss->selected = select;
        ss->visible = visible;
    }
}

static bool isSpecialConstructorOnlySelectionCase(int command) {
    if (command == OLO_PUSH) return true;
    if (command == OLO_PUSH_ONLY) return true;
    if (command == OLO_PUSH_AND_CALL_MACRO) return true;
    if (command == OLO_ARG_MANIP) return true;
    if (command == OLO_SAFETY_CHECK2) {
        // here check if origin was an argument manipulation
        OlcxReferences *refs, *origrefs, *newrefs, *diffrefs;
        int pbflag=0;
        origrefs = newrefs = diffrefs = NULL;
        SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
        assert(origrefs && newrefs && diffrefs);
        if (pbflag)
            return false;
        if (origrefs->command == OLO_ARG_MANIP)
            return true;
    }
    return false;
}

static void handleConstructorSpecialsInSelectingSymbolInMenu(SymbolsMenu *menu, int command) {
    SymbolsMenu *s1, *s2;
    int ccc, sss, lll, vn;

    if (!LANGUAGE(LANG_JAVA))
        return;
    if (!isSpecialConstructorOnlySelectionCase(command))
        return;
    s1 = NULL; s2 = NULL;
    vn = 0;
    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        if (ss->visible) {
            if (vn==0) s1 = ss;
            else if (vn==1) s2 = ss;
            vn ++;
        }
    }
    if (vn == 2) {
        // exactly two visible items
        assert(s1 && s2);
        sss = s1->selected + s2->selected;
        ccc = (s1->s.b.storage==StorageConstructor) + (s2->s.b.storage==StorageConstructor);
        lll = (s1->s.b.symType==TypeStruct) + (s2->s.b.symType==TypeStruct);
        //&fprintf(dumpOut," sss,ccc,lll == %d %d %d\n",sss,ccc,lll);
        if (sss==2 && ccc==1 && lll==1) {
            //dr1 = getDefinitionRef(s1->s.refs);
            //dr2 = getDefinitionRef(s2->s.refs);
            // deselect class references, but only if constructor definition exists
            // it was not a good idea with def refs, because of browsing of
            // javadoc for standard constructors
            if (s1->s.b.symType==TypeStruct /*& && dr2!=NULL &*/) {
                s1->selected = 0;
                if (command == OLO_ARG_MANIP) s1->visible = 0;
            } else if (s2->s.b.symType==TypeStruct /*& && dr1!=NULL &*/) {
                s2->selected = 0;
                if (command == OLO_ARG_MANIP) s2->visible = 0;
            }
        }
    }

}


#define RENAME_MENU_SELECTION(command) (                                \
                                        command == OLO_RENAME           \
                                        || command == OLO_ENCAPSULATE   \
                                        || command == OLO_VIRTUAL2STATIC_PUSH \
                                        || command == OLO_ARG_MANIP     \
                                        || command == OLO_PUSH_FOR_LOCALM \
                                        || command == OLO_SAFETY_CHECK1 \
                                        || command == OLO_SAFETY_CHECK2 \
                                        || options.manualResolve == RESOLVE_DIALOG_NEVER \
                                        )

static void computeSubClassOfRelatedItemsOOBit(SymbolsMenu *menu, int command) {
    unsigned oov;
    int st, change;

    if (RENAME_MENU_SELECTION(command)) {
        // even worse than O(n^2), hmm.
        change = 1;
        while (change) {
            change = 0;
            for (SymbolsMenu *s1=menu; s1!=NULL; s1=s1->next) {
                if ((s1->ooBits&OOC_VIRTUAL_MASK) < OOC_VIRT_SUBCLASS_OF_RELATED) goto nextrs1;
                for (SymbolsMenu *s2=menu; s2!=NULL; s2=s2->next) {
                    // do it only for virtuals
                    st = JAVA_STATICALLY_LINKED(s2->s.b.storage, s2->s.b.accessFlags);
                    if (st) goto nextrs2;
                    oov = (s2->ooBits & OOC_VIRTUAL_MASK);
                    if(oov >= OOC_VIRT_SUBCLASS_OF_RELATED) goto nextrs2;
                    if (isSmallerOrEqClass(s2->s.vApplClass, s1->s.vApplClass)) {
                        s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
                                      | OOC_VIRT_SUBCLASS_OF_RELATED);
                        change = 1;
                    }
                    if (isSmallerOrEqClass(s1->s.vApplClass, s2->s.vApplClass)) {
                        s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
                                      | OOC_VIRT_SUBCLASS_OF_RELATED);
                        change = 1;
                    }
                nextrs2:;
                }
            nextrs1:;
            }
        }
    } else {
        // O(n^2), someone should do this better !!!
        for (SymbolsMenu *s1=menu; s1!=NULL; s1=s1->next) {
            if ((s1->ooBits&OOC_VIRTUAL_MASK) < OOC_VIRT_RELATED) goto nexts1;
            for (SymbolsMenu *s2=menu; s2!=NULL; s2=s2->next) {
                // do it only for virtuals
                st = JAVA_STATICALLY_LINKED(s2->s.b.storage, s2->s.b.accessFlags);
                if (st) goto nexts2;
                oov = (s2->ooBits & OOC_VIRTUAL_MASK);
                if(oov >= OOC_VIRT_SUBCLASS_OF_RELATED) goto nexts2;
                if (isSmallerOrEqClass(s2->s.vApplClass, s1->s.vApplClass)) {
                    s2->ooBits = ((s2->ooBits & ~OOC_VIRTUAL_MASK)
                                  | OOC_VIRT_SUBCLASS_OF_RELATED);
                }
            nexts2:;
            }
        nexts1:;
        }
    }
}

static void setSelectedVisibleItems(SymbolsMenu *menu, int command, int filterLevel) {
    unsigned ooselected, oovisible;
    if (command == OLO_GLOBAL_UNUSED) {
        splitMenuPerSymbolsAndMap(menu, selectUnusedSymbols, &filterLevel, NULL);
        goto sfini;
    }
    // do not compute subclasses of related for class tree, it is too slow
    // and useless in this context
    if (command != OLO_CLASS_TREE) {
        computeSubClassOfRelatedItemsOOBit(menu, command);
    }
    if (command == OLO_MAYBE_THIS
        || command == OLO_NOT_FQT_REFS
        || command == OLO_NOT_FQT_REFS_IN_CLASS
        || command == OLO_USELESS_LONG_NAME
        || command == OLO_USELESS_LONG_NAME_IN_CLASS
        || command == OLO_PUSH_ALL_IN_METHOD
        || command == OLO_PUSH_NAME
        || command == OLO_PUSH_SPECIAL_NAME
        ) {
        // handle those very special cases first
        // set it to select and show all symbols
        oovisible = 0;
        ooselected = 0;
    } else if (RENAME_MENU_SELECTION(command)) {
        oovisible = options.ooChecksBits;
        ooselected = RENAME_SELECTION_OO_BITS;
    } else {
        if (s_olstringServed && s_olstringUsage == UsageMethodInvokedViaSuper) {
            //&oovisible = options.ooChecksBits;
            oovisible = s_menuFilterOoBits[filterLevel];
            ooselected = METHOD_VIA_SUPER_SELECTION_OO_BITS;
        } else {
            //&oovisible = options.ooChecksBits;
            oovisible = s_menuFilterOoBits[filterLevel];
            ooselected = DEFAULT_SELECTION_OO_BITS;
        }
    }
    setDefaultSelectedVisibleItems(menu, oovisible, ooselected);
    // for local motion a class browsing would make strange effect
    // if class is deselected on constructors
    handleConstructorSpecialsInSelectingSymbolInMenu(menu, command);
 sfini:
    return;
}

static void olcxMenuSelectPlusolcxMenuSelectFilterSet(int flevel) {
    OlcxReferences    *refs;

    OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    if (refs!=NULL && flevel < MAX_MENU_FILTER_LEVEL && flevel >= 0) {
        if (refs->menuFilterLevel != flevel) {
            refs->menuFilterLevel = flevel;
            setSelectedVisibleItems(refs->menuSym, refs->command, refs->menuFilterLevel);
            olcxRecomputeSelRefs(refs);
        }
    }
    if (refs!=NULL) {
        olcxPrintSelectionMenu(refs->menuSym);
        if (options.xref2) {
            // auto update of references
            // useless, should be done by user interface (because of setting
            // of reffilter level)
            //& olcxPrintRefList(";", refs);
        }
    }
    if (refs==NULL) {
        if (options.xref2) {
            olcxPrintSelectionMenu(NULL);
            olcxPrintRefList(";", NULL);
        } else {
            fprintf(communicationChannel, "=");
        }
    }
}

static void olcxReferenceFilterSet(int flevel) {
    OlcxReferences    *refs;

    OLCX_MOVE_INIT(currentUserData, refs, DEFAULT_VALUE);
    if (refs!=NULL && flevel < MAX_REF_LIST_FILTER_LEVEL && flevel >= 0) {
        refs->refsFilterLevel = flevel;
        //&     olcxPrintRefList(";", refs);
        //& } else {
        //&     olcxGenNoReferenceSignal();
    }
    if (options.xref2) {
        // move to the visible reference
        if (refs!=NULL) olcxSetActReferenceToFirstVisible(refs, refs->actual);
        olcxPrintRefList(";", refs);
    } else {
        fprintf(communicationChannel, "*");
    }
}

static OlcxReferences *getNextTopStackItem(OlcxReferencesStack *stack) {
    OlcxReferences *rr, *nextrr;
    nextrr = NULL;
    rr = stack->root;
    while (rr!=NULL && rr!=stack->top) {
        nextrr = rr;
        rr = rr->previous;
    }
    assert(rr==stack->top);
    return nextrr;
}

static void olcxReferenceRePush(void) {
    OlcxReferences *refs, *nextrr;

    OLCX_MOVE_INIT(currentUserData,refs, DEFAULT_VALUE);
    nextrr = getNextTopStackItem(&currentUserData->browserStack);
    if (nextrr != NULL) {
        currentUserData->browserStack.top = nextrr;
        olcxGenGotoActReference(currentUserData->browserStack.top);
        // TODO, replace this by follwoing since 1.6.1
        //& ppcGotoPosition(&currentUserData->browserStack.top->callerPosition);
        olcxPrintSymbolName(currentUserData->browserStack.top);
    } else {
        if (options.xref2) {
            ppcBottomWarning("You are on the top of browser stack.");
        } else {
            fprintf(communicationChannel, "*** Complete stack, no pop-ed references");
        }
    }
}

static void olcxReferencePop(void) {
    OlcxReferences *refs;
    OLCX_MOVE_INIT(currentUserData, refs, CHECK_NULL);
    if (refs->callerPosition.file != noFileIndex) {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, "");
    } else {
        olcxGenNoReferenceSignal();
    }
    //& olStackDeleteSymbol(refs);  // this was before non deleting pop
    currentUserData->browserStack.top = refs->previous;
    olcxPrintSymbolName(currentUserData->browserStack.top);
}

void olcxPopOnly(void) {
    OlcxReferences *refs;

    OLCX_MOVE_INIT(currentUserData,refs,CHECK_NULL);
    if (!options.xref2) fprintf(communicationChannel, "*");
    //& olStackDeleteSymbol(refs);
    currentUserData->browserStack.top = refs->previous;
}

Reference * olcxCopyRefList(Reference *ll) {
    Reference *res, *a, **aa;
    res = NULL; aa= &res;
    for (Reference *rr=ll; rr!=NULL; rr=rr->next) {
        a = olcx_alloc(sizeof(Reference));
        *a = *rr;
        a->next = NULL;
        *aa = a;
        aa = &(a->next);
    }
    return res;
}

static void safetyCheckAddDiffRef(Reference *r, OlcxReferences *diffrefs,
                                  int mode) {
    int prefixchar;
    prefixchar = ' ';
    if (diffrefs->references == NULL) {
        fprintf(communicationChannel, "%s", COLCX_LIST);
        prefixchar = '>';
    }
    if (mode == DIFF_MISSING_REF) {
        fprintf(communicationChannel, "%c %s:%d missing reference\n", prefixchar,
                simpleFileNameFromFileNum(r->position.file), r->position.line);
    } else if (mode == DIFF_UNEXPECTED_REF) {
        fprintf(communicationChannel, "%c %s:%d unexpected new reference\n", prefixchar,
                simpleFileNameFromFileNum(r->position.file), r->position.line);
    } else {
        assert(0);
    }
    olcxAppendReference(r, diffrefs);
}

void olcxReferencesDiff(Reference **anr1,
                        Reference **aor2,
                        Reference **diff
                        ) {
    Reference *r, *nr1, *or2, **dd;

    LIST_MERGE_SORT(Reference, *anr1, olcxReferenceInternalLessFunction);
    LIST_MERGE_SORT(Reference, *aor2, olcxReferenceInternalLessFunction);
    nr1 = *anr1; or2 = *aor2; dd = diff;
    *dd = NULL;
    while (nr1!=NULL && or2!=NULL) {
        if (positionsAreEqual(nr1->position, or2->position)) {
            nr1 = nr1->next; or2=or2->next;
        } else {
            if (SORTED_LIST_LESS(nr1, *or2)) {
                *dd = olcxCopyReference(nr1);
                dd = &(*dd)->next;
                nr1=nr1->next;
            } else {
                *dd = olcxCopyReference(or2);
                dd = &(*dd)->next;
                or2=or2->next;
            }
        }
    }
    if (nr1!=NULL || or2!=NULL) {
        if (nr1!=NULL) {
            r = nr1;
            /* mode = DIFF_UNEXPECTED_REF; */
        } else {
            r = or2;
            /* mode = DIFF_MISSING_REF; */
        }
        for (; r!=NULL; r=r->next) {
            *dd = olcxCopyReference(r);
            dd = &(*dd)->next;
        }
    }
}

static void safetyCheckDiff(Reference **anr1,
                            Reference **aor2,
                            OlcxReferences *diffrefs
                            ) {
    Reference *r, *nr1, *or2;
    int mode;
    LIST_MERGE_SORT(Reference, *anr1, olcxReferenceInternalLessFunction);
    LIST_MERGE_SORT(Reference, *aor2, olcxReferenceInternalLessFunction);
    nr1 = *anr1; or2 = *aor2;
    while (nr1!=NULL && or2!=NULL) {
        if (nr1->position.file==or2->position.file && nr1->position.line==or2->position.line) {
            nr1 = nr1->next; or2=or2->next;
        } else {
            if (SORTED_LIST_LESS(nr1, *or2)) {
                safetyCheckAddDiffRef(nr1, diffrefs, DIFF_UNEXPECTED_REF);
                nr1=nr1->next;
            } else {
                safetyCheckAddDiffRef(or2, diffrefs, DIFF_MISSING_REF);
                or2=or2->next;
            }
        }
    }
    if (nr1!=NULL || or2!=NULL) {
        if (nr1!=NULL) {
            r = nr1;
            mode = DIFF_UNEXPECTED_REF;
        } else {
            r = or2;
            mode = DIFF_MISSING_REF;
        }
        for (; r!=NULL; r=r->next) {
            safetyCheckAddDiffRef(r, diffrefs, mode);
        }
    }
    diffrefs->actual = diffrefs->references;
    if (diffrefs->references!=NULL) {
        assert(diffrefs->menuSym);
        olcxAddReferencesToSymbolsMenu(diffrefs->menuSym, diffrefs->references, 0);
    }
}

int getFileNumberFromName(char *name) {
    char *normalizedName;
    int fileIndex;

    normalizedName = normalizeFileName(name, cwd);
    if ((fileIndex = fileTableLookup(&fileTable, normalizedName)) != -1) {
        return fileIndex;
    } else {
        return noFileIndex;
    }
}

static Reference *olcxCreateFileShiftedRefListForCheck(Reference *rr) {
    Reference *res, **resa;
    int ofn, nfn, fmline, lmline;

    //&fprintf(dumpOut,"!shifting enter\n");
    if (options.checkFileMovedFrom==NULL) return NULL;
    if (options.checkFileMovedTo==NULL) return NULL;
    //&fprintf(dumpOut,"!shifting %s --> %s\n", options.checkFileMovedFrom, options.checkFileMovedTo);
    ofn = getFileNumberFromName(options.checkFileMovedFrom);
    nfn = getFileNumberFromName(options.checkFileMovedTo);
    //&fprintf(dumpOut,"!shifting %d --> %d\n", ofn, nfn);
    if (ofn==noFileIndex) return NULL;
    if (nfn==noFileIndex) return NULL;
    fmline = options.checkFirstMovedLine;
    lmline = options.checkFirstMovedLine + options.checkLinesMoved;
    res = NULL; resa = &res;
    for (Reference *r=rr; r!=NULL; r=r->next) {
        Reference *tt = olcx_alloc(sizeof(Reference));
        *tt = *r;
        if (tt->position.file==ofn && tt->position.line>=fmline && tt->position.line<lmline) {
            tt->position.file = nfn;
            //&fprintf(dumpOut,"!shifting %d:%d to %d (%d %d %d)\n", tt->position.file, tt->position.line, options.checkNewLineNumber + (tt->position.line - fmline), options.checkFirstMovedLine, options.checkLinesMoved, options.checkNewLineNumber);
            tt->position.line = options.checkNewLineNumber + (tt->position.line - fmline);
        }
        *resa=tt; tt->next=NULL; resa= &(tt->next);
    }
    LIST_MERGE_SORT(Reference, res, olcxReferenceInternalLessFunction);
    return res;
}

static void olcxSafetyCheck2(void) {
    OlcxReferences *refs, *origrefs, *newrefs, *diffrefs;
    Reference *shifted;
    int pbflag=0;
    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
    assert(origrefs && newrefs && diffrefs);
    if (pbflag) return;
    shifted = olcxCreateFileShiftedRefListForCheck(origrefs->references);
    if (shifted != NULL) {
        safetyCheckDiff(&newrefs->references, &shifted, diffrefs);
        olcxFreeReferences(shifted);
    } else {
        safetyCheckDiff(&newrefs->references, &origrefs->references, diffrefs);
    }
    if (diffrefs->references == NULL) {
        // no need to free here, as popings are not freed
        currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
        currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
        currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
        fprintf(communicationChannel, "*Done. No conflicts detected.");
    } else {
        assert(diffrefs->menuSym);
        currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
        fprintf(communicationChannel, " ** Some misinterpreted references detected. Please, undo last refactoring.");
    }
    fflush(communicationChannel);
}

static bool olRemoveCallerReference(OlcxReferences *refs) {
    Reference *rr, **rrr;
    LIST_MERGE_SORT(Reference, refs->references, olcxReferenceInternalLessFunction);
    for (rrr= &refs->references, rr=refs->references; rr!=NULL; rrr= &rr->next, rr=rr->next){
        //&fprintf(dumpOut,"checking %d %d %d to %d %d %d\n",rr->position.file, rr->position.line,rr->position.col, refs->callerPosition.file,  refs->callerPosition.line,  refs->callerPosition.col);
        if (! positionIsLessThan(rr->position, refs->callerPosition))
            break;
    }
    if (rr == NULL)
        return false;
    if (! IS_DEFINITION_OR_DECL_USAGE(rr->usage.base))
        return false;
    //&fprintf(dumpOut,"!removing reference on %d\n", rr->position.line);
    *rrr = rr->next;
    olcx_memory_free(rr, sizeof(Reference));

    return true;
}

static void olEncapsulationSafetyCheck(void) {
    OlcxReferences *refs;
    assert(currentUserData);
    refs = currentUserData->browserStack.top;
    if (refs==NULL || refs->previous==NULL){
        errorMessage(ERR_INTERNAL,"something goes wrong at encapsulate safety check");
        return;
    }
    // remove definition reference, so they do not interfere
    assert(currentUserData->browserStack.top!=NULL && currentUserData->browserStack.top->previous!=NULL
           && currentUserData->browserStack.top->previous->previous!=NULL
           && currentUserData->browserStack.top->previous->previous->previous!=NULL);
    olRemoveCallerReference(currentUserData->browserStack.top);
    olRemoveCallerReference(currentUserData->browserStack.top->previous);
    olRemoveCallerReference(currentUserData->browserStack.top->previous->previous->previous);
    // join references from getter and setter and make regular safety check
    olcxAddReferences(refs->references, &currentUserData->browserStack.top->previous->references,
                      ANY_FILE, 0);
    currentUserData->browserStack.top = currentUserData->browserStack.top->previous;
    olcxSafetyCheck2();
}

static void olCompletionSelect(void) {
    OlcxReferences    *refs;
    S_olCompletion      *rr;
    OLCX_MOVE_INIT(currentUserData,refs, CHECK_NULL);
    rr = olCompletionNthLineRef(refs->completions, options.olcxGotoVal);
    if (rr==NULL) {
        errorMessage(ERR_ST, "selection out of range.");
        return;
    }
    if (options.xref2) {
        assert(currentUserData->completionsStack.root!=NULL);
        ppcGotoPosition(&currentUserData->completionsStack.root->callerPosition);
        if (rr->csymType==TypeNonImportedClass) {
            ppcGenRecord(PPC_FQT_COMPLETION, rr->name);
        } else {
            ppcGenRecord(PPC_SINGLE_COMPLETION, rr->name);
        }
    } else {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, rr->name);
    }
    //& olStackDeleteSymbol(refs);
}

static void olcxReferenceSelectTagSearchItem(int refn) {
    S_olCompletion      *rr;
    OlcxReferences    *refs;
    char                ttt[MAX_FUN_NAME_SIZE];
    assert(refn > 0);
    assert(currentUserData);
    assert(currentUserData->retrieverStack.top);
    refs = currentUserData->retrieverStack.top;
    rr = olCompletionNthLineRef(refs->completions, refn);
    if (rr == NULL) {
        errorMessage(ERR_ST, "selection out of range.");
        return;
    }
    assert(currentUserData->retrieverStack.root!=NULL);
    ppcGotoPosition(&currentUserData->retrieverStack.root->callerPosition);
    sprintf(ttt, " %s", rr->name);
    ppcGenRecord(PPC_SINGLE_COMPLETION, ttt);
}

static void olCompletionBack(void) {
    OlcxReferences    *top;
    assert(currentUserData);
    top = currentUserData->completionsStack.top;
    if (top != NULL && top->previous != NULL) {
        currentUserData->completionsStack.top = currentUserData->completionsStack.top->previous;
        ppcGotoPosition(&currentUserData->completionsStack.top->callerPosition);
        printCompletionsList(0);
    }
}

static void olCompletionForward(void) {
    OlcxReferences    *top;
    assert(currentUserData);
    top = getNextTopStackItem(&currentUserData->completionsStack);
    if (top != NULL) {
        currentUserData->completionsStack.top = top;
        ppcGotoPosition(&currentUserData->completionsStack.top->callerPosition);
        printCompletionsList(0);
    }
}

static void olcxNoSymbolFoundErrorMessage(void) {
    if (options.server_operation == OLO_PUSH_NAME || options.server_operation == OLO_PUSH_SPECIAL_NAME) {
        if (options.xref2) {
            ppcGenRecord(PPC_ERROR,"No symbol found.");
        } else {
            fprintf(communicationChannel,"*** No symbol found.");
        }
    } else {
        if (options.xref2) {
            ppcGenRecord(PPC_ERROR,"No symbol found, please position the cursor on a program symbol.");
        } else {
            fprintf(communicationChannel,"*** No symbol found, please position the cursor on a program symbol.");
        }
    }
}


static bool olcxCheckSymbolExists(void) {
    if (currentUserData!=NULL
        && currentUserData->browserStack.top!=NULL
        && currentUserData->browserStack.top->menuSym==NULL) {
        return false;
    }
    return true;
}

static SymbolsMenu *firstVisibleSymbol(SymbolsMenu *first) {
    SymbolsMenu *fvisible = NULL;

    for (SymbolsMenu *ss=first; ss!=NULL; ss=ss->next) {
        if (ss->visible) {
            fvisible = ss;
            break;
        }
    }
    return fvisible;
}

static bool staticallyLinkedSymbolMenu(SymbolsMenu *menu) {
    SymbolsMenu *fv;
    fv = firstVisibleSymbol(menu);
    if (JAVA_STATICALLY_LINKED(fv->s.b.storage,fv->s.b.accessFlags)) {
        return true;
    } else {
        return false;
    }
}

bool olcxShowSelectionMenu(void) {
    SymbolsMenu *first, *fvisible;

    // decide whether to show manual resolution menu
    assert(currentUserData && currentUserData->browserStack.top);
    if (options.server_operation == OLO_PUSH_FOR_LOCALM) {
        // never ask for resolution for local motion symbols
        return false;
    }
    if (options.server_operation == OLO_SAFETY_CHECK2) {
        // safety check showing of menu is resolved by safetyCheck2ShouldWarn
        return false;
    }
    // first if just zero or one symbol, no resolution
    first = currentUserData->browserStack.top->menuSym;
    if (first == NULL) {
        //&fprintf(dumpOut,"no resolve, no symbol\n"); fflush(dumpOut);
        return false; // no symbol
    }
    fvisible = firstVisibleSymbol(first);
    if (fvisible==NULL) {
        //&fprintf(dumpOut,"no resolve, no visible\n"); fflush(dumpOut);
        return false; // no visible
    }
    first = NULL;
    if (options.server_operation==OLO_PUSH
        || options.server_operation==OLO_PUSH_ONLY
        || options.server_operation==OLO_PUSH_AND_CALL_MACRO
        || options.server_operation==OLO_RENAME
        || options.server_operation==OLO_ARG_MANIP
        || options.server_operation==OLO_PUSH_ENCAPSULATE_SAFETY_CHECK
        || JAVA_STATICALLY_LINKED(fvisible->s.b.storage,
                                  fvisible->s.b.accessFlags)) {
        // manually only if different
        for (SymbolsMenu *ss=currentUserData->browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
            if (ss->selected) {
                if (first == NULL) {
                    first = ss;
                } else if ((! isSameCxSymbol(&first->s, &ss->s))
                           || first->s.vFunClass!=ss->s.vFunClass) {
                    return true;
                }
            }
        }
    } else {
        for (SymbolsMenu *ss=currentUserData->browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
            if (ss->visible) {
                if (first!=NULL) {
                    return true;
                }
                first = ss;
            }
        }
    }
    return false;
}

static int countSelectedItems(SymbolsMenu *menu) {
    int count = 0;
    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        if (ss->selected)
            count++;
    }
    return count;
}

static bool refactoringLinkNameCorrespondance(char *n1, char *n2, int command) {
    // this should say if there is refactoring link names correspondance
    // between n1 and n2
    //&fprintf(dumpOut,"checking lnc between %s %s on %s\n", n1, n2, olcxOptionsName[command]);
    //& if (command == OLO_RENAME) return linkNamesHaveSameProfile(n1, n2);
    //& if (command == OLO_ENCAPSULATE) return linkNamesHaveSameProfile(n1, n2);
    //& if (command == OLO_ARG_MANIP) return linkNamesHaveSameProfile(n1, n2);
    //& if (command == OLO_VIRTUAL2STATIC_PUSH) return linkNamesHaveSameProfile(n1, n2);
    // TODO!! other cases (argument manipulations) not yet implemented
    // they are all as OLO_RENAME for now !!!
    return true;
}

static SymbolsMenu *safetyCheck2FindCorrMenuItem(SymbolsMenu *item,
                                                 SymbolsMenu *menu,
                                                 int command
) {
    for (SymbolsMenu *ss=menu; ss!=NULL; ss=ss->next) {
        if (ss->selected
            && ss->s.vApplClass == item->s.vApplClass
            && ss->s.b.symType == item->s.b.symType
            && ss->s.b.storage == item->s.b.storage
            && refactoringLinkNameCorrespondance(ss->s.name, item->s.name,
                                                 command)) {
            // here it is
            return ss;
        }
    }
    return NULL;
}

static bool scCompareVirtualHierarchies(SymbolsMenu *origm,
                                        SymbolsMenu *newm,
                                        int command) {
    int count1, count2;
    int res = false;

    count1 = countSelectedItems(origm);
    count2 = countSelectedItems(newm);
    //&fprintf(dumpOut,":COMPARING\n");olcxPrintSelectionMenu(origm);fprintf(dumpOut,":TO\n");olcxPrintSelectionMenu(newm);fprintf(dumpOut,":END\n");
    if (count1 != count2)
        res = true;
    for (SymbolsMenu *ss=newm; ss!=NULL; ss=ss->next) {
        if (ss->selected) {
            SymbolsMenu *oss = safetyCheck2FindCorrMenuItem(ss, origm, command);
            if (oss==NULL || oss->selected == 0) {
                res = true;
                ss->selected = 0;
            }
        }
    }
    return res;
}

bool safetyCheck2ShouldWarn(void) {
    int res,problem;
    OlcxReferences *refs, *origrefs, *newrefs, *diffrefs;
    problem = 0;
    if (options.server_operation != OLO_SAFETY_CHECK2) return false;
    if (! LANGUAGE(LANG_JAVA)) return false;
    // compare hierarchies and deselect diff
    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, problem);
    if (problem) return false;
    assert(origrefs && newrefs && diffrefs);
    if (staticallyLinkedSymbolMenu(origrefs->menuSym)) return false;
    res = scCompareVirtualHierarchies(origrefs->menuSym, newrefs->menuSym,
                                      origrefs->command);
    if (res) {
        // hierarchy was changed, recompute refs
        olcxRecomputeSelRefs(newrefs);
    }
    return res;
}

static bool olMenuHashFileNumLess(SymbolsMenu *s1, SymbolsMenu *s2) {
    int fi1, fi2;
    fi1 = cxFileHashNumber(s1->s.name);
    fi2 = cxFileHashNumber(s2->s.name);
    if (fi1 < fi2) return true;
    if (fi1 > fi2) return false;
    if (s1->s.b.category == CategoryLocal) return true;
    if (s1->s.b.category == CategoryLocal) return false;
    // both files and categories equals ?
    return false;
}

void getLineColCursorPositionFromCommandLineOption(int *l, int *c) {
    assert(options.olcxlccursor!=NULL);
    sscanf(options.olcxlccursor,"%d:%d", l, c);
}

int getClassNumFromClassLinkName(char *name, int defaultResult) {
    int fileIndex;
    char classFileName[MAX_FILE_NAME_SIZE];

    fileIndex = defaultResult;
    convertLinkNameToClassFileName(classFileName, name);
    log_trace("Looking for class file '%s'", classFileName);
    if (fileTableExists(&fileTable, classFileName))
        fileIndex = fileTableLookup(&fileTable, classFileName);

    return fileIndex;
}

static int olSpecialFieldCreateSelection(char *fieldName, int storage) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;
    int                 clii;
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    if (! LANGUAGE(LANG_JAVA)) {
        rstack->hkSelectedSym = NULL;
        errorMessage(ERR_ST,"This function is available only in Java language");
        return noFileIndex;
    }
    assert(s_javaObjectSymbol && s_javaObjectSymbol->u.structSpec);
    clii = s_javaObjectSymbol->u.structSpec->classFile;
    //&fprintf(dumpOut,"class clii==%d==%s\n",clii,fileTable.tab[clii]->name);fflush(dumpOut);
    ss = rstack->hkSelectedSym;
    if (ss!=NULL && ss->next!=NULL) {
        // several cursor selected fields
        // probably constructor, look for a class type
        if (ss->next->s.b.symType == TypeStruct) ss = ss->next;
    }
    if (ss != NULL) {
        //&fprintf(dumpOut, "sym %s of %s\n", ss->s.name, typeNamesTable[ss->s.b.symType]);
        if (ss->s.b.symType == TypeStruct) {
            clii = getClassNumFromClassLinkName(ss->s.name, clii);
        } else {
            if (options.server_operation == OLO_CLASS_TREE) {
                assert(currentUserData->classTree.baseClassFileIndex!=noFileIndex);
                clii = currentUserData->classTree.baseClassFileIndex;
            } else {
                if (ss->s.vApplClass!=noFileIndex) clii = ss->s.vApplClass;
            }
        }
    }
    //&fprintf(dumpOut,"class clii==%d==%s\n",clii,fileTable.tab[clii]->name);fflush(dumpOut);
    olcxFreeResolutionMenu(ss);
    rstack->hkSelectedSym = olCreateSpecialMenuItem(fieldName, clii, storage);
    return clii;
}


void olCreateSelectionMenu(int command) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;
    int                 fnum;

    // I think this ordering is useless
    LIST_MERGE_SORT(SymbolsMenu,
                    currentUserData->browserStack.top->hkSelectedSym,
                    refItemsOrderLess);
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    if (ss == NULL) return;
    renameCollationSymbols(ss);
    LIST_SORT(SymbolsMenu, rstack->hkSelectedSym, olMenuHashFileNumLess);
    ss = rstack->hkSelectedSym;
    while (ss!=NULL) {
        readOneAppropReferenceFile(ss->s.name, symbolMenuCreationFunctionSequence);
        fnum = cxFileHashNumber(ss->s.name);
        //&fprintf(dumpOut,"file %d readed\n", fnum);
        while (ss!=NULL && fnum==cxFileHashNumber(ss->s.name)) ss = ss->next;
    }
    refTabMap(&referenceTable, mapCreateSelectionMenu);
    refTabMap(&referenceTable, putOnLineLoadedReferences);
    setSelectedVisibleItems(rstack->menuSym, command, rstack->menuFilterLevel);
    assert(rstack->references==NULL);
    olProcessSelectedReferences(rstack, genOnLineReferences);
    // isn't ordering useless ?
    LIST_MERGE_SORT(SymbolsMenu,
                    currentUserData->browserStack.top->menuSym,
                    refItemsOrderLess);
}

bool refOccursInRefs(Reference *r, Reference *list) {
    Reference *place;
    SORTED_LIST_FIND2(place,Reference, (*r),list);
    if (place==NULL || SORTED_LIST_NEQ(place, *r))
        return false;
    return true;
}

static void olcxSingleReferenceCheck1(SymbolReferenceItem *p,
                                      OlcxReferences *rstack,
                                      Reference *r
) {
    int prefixchar;

    if (refOccursInRefs(r, rstack->references)) {
        prefixchar = ' ';
        if (currentUserData->browserStack.top->references == NULL) {
            fprintf(communicationChannel,"%s",COLCX_LIST);
            prefixchar = '>';
        }
        fprintf(communicationChannel,"%c  %s:%d reference to '", prefixchar,
                simpleFileNameFromFileNum(r->position.file), r->position.line);
        printSymbolLinkNameString(communicationChannel, p->name);
        fprintf(communicationChannel,"' lost\n");
        olcxAppendReference(r, currentUserData->browserStack.top);
    }
}

void olcxCheck1CxFileReference(SymbolReferenceItem *ss, Reference *r) {
    SymbolReferenceItem     *sss;
    OlcxReferences    *rstack;
    SymbolsMenu     *cms;
    bool pushed;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top->previous;
    assert(rstack && rstack->menuSym);
    sss = &rstack->menuSym->s;
    //&fprintf(dumpOut,"checking 1 file refs %s at %s:%d\n", ss->name,simpleFileNameFromFileNum(r->position.file),r->position.line);
    pushed = itIsSymbolToPushOlReferences(ss, rstack, &cms, DEFAULT_VALUE);
    // this is very slow to check the symbol name for each reference
    if (pushed == 0 && olcxIsSameCxSymbol(ss, sss)) {
        olcxSingleReferenceCheck1(ss, rstack, r);
    }
}

static void olcxProceedSafetyCheck1OnInloadedRefs(OlcxReferences *rstack, SymbolsMenu *ccms) {
    SymbolReferenceItem     *p;
    SymbolReferenceItem     *sss;
    SymbolsMenu     *cms;
    bool pushed;

    p = &ccms->s;
    assert(rstack && rstack->menuSym);
    sss = &rstack->menuSym->s;
    pushed = itIsSymbolToPushOlReferences(p, rstack, &cms, DEFAULT_VALUE);
    // TODO, this can be simplified, as ccms == cms.
    //&fprintf(dumpOut,":checking %s to %s (%d)\n",p->name, sss->name, pushed);
    if (!pushed && olcxIsSameCxSymbol(p, sss)) {
        //&fprintf(dumpOut,"checking %s references\n",p->name);
        for (Reference *r=p->refs; r!=NULL; r=r->next) {
            olcxSingleReferenceCheck1(p, rstack, r);
        }
    }
}

void olcxPushSpecialCheckMenuSym(char *symname) {
    OlcxReferences *rstack;
    olcxSetCurrentUser(options.user);
    olcxPushEmptyStackItem(&currentUserData->browserStack);
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(symname, noFileIndex, StorageDefault);
    rstack->menuSym = olCreateSpecialMenuItem(symname, noFileIndex, StorageDefault);
}

static void olcxSafetyCheckInit(void) {
    assert(options.server_operation == OLO_SAFETY_CHECK_INIT);
    olcxPushSpecialCheckMenuSym(LINK_NAME_SAFETY_CHECK_MISSED);
    fprintf(communicationChannel,"* safety checks initialized");
    fflush(communicationChannel);
}

static SymbolsMenu *mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(
                                                                          SymbolsMenu *menuSym) {
    SymbolsMenu *res = NULL;

    for (SymbolsMenu *mm=menuSym; mm!=NULL; mm=mm->next) {
        if (mm->s.refs!=NULL && IS_DEFINITION_OR_DECL_USAGE(mm->s.refs->usage.base) &&
            (res==NULL || positionIsLessThan(mm->s.refs->position, res->s.refs->position))) {
            res = mm;
        }
    }
    return res;
}

static SymbolsMenu *mmFindSymWithCorrespondingRef(Reference *ref,
                                                      SymbolsMenu *osym,
                                                      OlcxReferences *refs,
                                                      Position *moveOffset
) {
    Reference sr, *place;

    sr = *ref;
    sr.position = addPositions(ref->position, *moveOffset);
    // now looks for the reference 'r'
    for (SymbolsMenu *mm=refs->menuSym; mm!=NULL; mm=mm->next) {
        // do not check anything, but symbol type to avoid missresolution
        // of ambiguous references class <-> constructor
        //&fprintf(dumpOut,";looking for correspondance %s <-> %s\n", osym->s.name,mm->s.name);
        if (mm->s.b.symType != osym->s.b.symType
            && mm->s.b.symType != TypeInducedError) continue;
        // check also that maybe this references are not mixed with regular
        //&if (mm->s.name[0]==' ' || osym->s.name[0]==' ') {
        //& if (mm->s.name[0]!=' ' || osym->s.name[0]!=' ') continue;
        //&}
        //&fprintf(dumpOut,";looking references\n");
        SORTED_LIST_FIND2(place,Reference, sr, mm->s.refs);
        if (place!=NULL && ! SORTED_LIST_NEQ(place, sr)) {
            // I have got it
            //&fprintf(dumpOut,";I have got it!\n");
            return mm;
        }
    }
    return NULL;
}

bool symbolsCorrespondWrtMoving(SymbolsMenu *osym,
                               SymbolsMenu *nsym,
                               int command) {
    bool res = false;

    switch (command) {
    case OLO_MM_PRE_CHECK:
        if (isSameCxSymbol(&osym->s, &nsym->s)
            && osym->s.vApplClass == nsym->s.vApplClass) {
            res = true;
        }
        break;
    case OLO_PP_PRE_CHECK:
        if (isSameCxSymbol(&osym->s, &nsym->s)) {
            if (osym->s.vApplClass == nsym->s.vApplClass) {
                res = true;
            }
            if (osym->s.b.storage == StorageField
                && osym->s.vFunClass == nsym->s.vFunClass) {
                res = true;
            }
            if (osym->s.b.storage == StorageMethod) {
                res = true;
            }
        }
        break;
    default:
        assert(0);
    }
    // do not report misinterpretations induced by previous errors
    if (nsym->s.b.symType == TypeInducedError)
        res = true;
    //&fprintf(dumpOut,";checking corr. %s %s res==%d\n", osym->s.name, nsym->s.name, res);
    return res;
}

static bool mmPreCheckMakeDifference(OlcxReferences *origrefs,
                                     OlcxReferences *newrefs,
                                     OlcxReferences *diffrefs
) {
    SymbolsMenu *nsym, *diffsym, *ofirstsym, *nfirstsym;
    Position moveOffset;

    moveOffset = makePosition(0, 0, 0);
    ofirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(origrefs->menuSym);
    nfirstsym = mmPreCheckGetFirstDefinitionReferenceAndItsSymbol(newrefs->menuSym);
    if (ofirstsym!=NULL && nfirstsym!=NULL) {
        // TODO! Check here rather symbol name, than just column offsets
        assert(ofirstsym->s.refs && nfirstsym->s.refs);
        moveOffset = subtractPositions(nfirstsym->s.refs->position, ofirstsym->s.refs->position);
        if (moveOffset.col!=0) {
            errorMessage(ERR_ST, "method has to be moved into an empty line");
            return true;
        }
    }
    //&fprintf(dumpOut,": line Offset == %d\n", lineOffset);
    for (SymbolsMenu *osym = origrefs->menuSym; osym!=NULL; osym=osym->next) {
        //&fprintf(dumpOut,";check on %s\n", osym->s.name);
        // do not check recursive calls
        if (osym == ofirstsym) goto cont;
        // nor local variables
        if (osym->s.b.storage == StorageAuto) goto cont;
        // nor labels
        if (osym->s.b.symType == TypeLabel) goto cont;
        // do not check also any symbols from classes defined in inner scope
        if (isStrictlyEnclosingClass(osym->s.vFunClass, ofirstsym->s.vFunClass)) goto cont;
        // (maybe I should not test any local symbols ???)

        // check the symbol
        diffsym = NULL;
        for (Reference *rr=osym->s.refs; rr!=NULL; rr=rr->next) {
            nsym = mmFindSymWithCorrespondingRef(rr,osym,newrefs,&moveOffset);
            if (nsym==NULL || !symbolsCorrespondWrtMoving(osym, nsym, options.server_operation)) {
                if (diffsym == NULL) {
                    diffsym = olAddBrowsedSymbol(
                                                 &osym->s, &diffrefs->menuSym, 1, 1,
                                                 (OOC_PROFILE_EQUAL|OOC_VIRT_SAME_FUN_CLASS),
                                                 USAGE_ANY, 0, &noPosition, UsageNone);
                }
                olcxAddReferenceToSymbolsMenu(diffsym, rr, 0);
            }
        }
    cont:;
    }
    return false;
}

static void olcxMMPreCheck(void) {
    OlcxReferences    *diffrefs, *origrefs, *newrefs;
    SymbolReferenceItem     dri;
    bool precheck;

    olcxSetCurrentUser(options.user);
    olcxPushEmptyStackItem(&currentUserData->browserStack);
    assert(currentUserData && currentUserData->browserStack.top);
    assert(options.server_operation == OLO_MM_PRE_CHECK || options.server_operation == OLO_PP_PRE_CHECK);
    diffrefs = currentUserData->browserStack.top;
    assert(diffrefs && diffrefs->previous && diffrefs->previous->previous);
    newrefs = diffrefs->previous;
    origrefs = newrefs->previous;
    precheck = mmPreCheckMakeDifference(origrefs, newrefs, diffrefs);
    olStackDeleteSymbol(origrefs);
    olStackDeleteSymbol(newrefs);
    if (precheck==0) {
        if (diffrefs->menuSym!=NULL) {
            fillTrivialSpecialRefItem(&dri, "  references missinterpreted after refactoring");
            olAddBrowsedSymbol(&dri, &diffrefs->hkSelectedSym, 1, 1, 0,
                               USAGE_ANY, 0, &noPosition, UsageNone);
            olProcessSelectedReferences(diffrefs, genOnLineReferences);
            //&olcxPrintSelectionMenu(diffrefs->menuSym);
            olcxPrintRefList(";", diffrefs);
        } else {
            olStackDeleteSymbol(diffrefs);
            fprintf(communicationChannel,"* Method moving pre-check passed.");
        }
    }
    fflush(communicationChannel);
}


static void olcxSafetyCheck1(void) {
    OlcxReferences    *rstack;
    // in reality this is a hack, it takes references kept from
    // last file processing
    assert(currentUserData && currentUserData->browserStack.top);
    assert(currentUserData->browserStack.top->previous);
    assert(options.server_operation == OLO_SAFETY_CHECK1);
    rstack = currentUserData->browserStack.top->previous;
    olProcessSelectedReferences(rstack, olcxProceedSafetyCheck1OnInloadedRefs);
    if (currentUserData->browserStack.top->references == NULL) {
        fprintf(communicationChannel,"* check1 passed");
    } else {
        currentUserData->browserStack.top->actual = currentUserData->browserStack.top->references;
        fprintf(communicationChannel," ** Shared references lost. Please, undo last refactoring\n");
    }
    fflush(communicationChannel);
}

static bool refOccursInRefsCompareFileAndLineOnly(Reference *rr,
                                                  Reference *list
) {
    for (Reference *r=list; r!=NULL; r=r->next) {
        if (r->position.file==rr->position.file && r->position.line==rr->position.line)
            return true;
    }
    return false;
}

static void olcxTopReferencesIntersection(void) {
    OlcxReferences    *top1,*top2;
    Reference         **r1,**r, *nr;
    // in reality this is a hack, it takes references kept from
    // last file processing
    assert(currentUserData && currentUserData->browserStack.top);
    assert(currentUserData->browserStack.top->previous);
    assert(options.server_operation == OLO_INTERSECTION);
    top1 = currentUserData->browserStack.top;
    top2 = currentUserData->browserStack.top->previous;
    //TODO in linear time, not O(n^2) like now.
    r1 = & top1->references;
    while (*r1!=NULL) {
        r = r1; r1 = &(*r1)->next;
        if (!refOccursInRefsCompareFileAndLineOnly(*r, top2->references)) {
            // remove the reference
            nr = *r1;
            olcx_memory_free(*r, sizeof(Reference));
            *r = nr;
            r1 = r;
        }
    }
    top1->actual = top1->references;
    fprintf(communicationChannel,"*");
}

static void olcxRemoveRefWinFromRefList(Reference **r1,
                                        int wdfile,
                                        Position *fp,
                                        Position *tp ) {
    Reference **r, *cr, *nr;
    while (*r1!=NULL) {
        r = r1; r1 = &(*r1)->next;
        cr = *r;
        //&fprintf(dumpOut,"! checking %d:%d\n", cr->position.line, cr->position.col);
        if (cr->position.file == wdfile
            && positionIsLessThan(*fp, cr->position) && positionIsLessThan(cr->position, *tp)) {
            // remove the reference
            nr = *r1;
            olcx_memory_free(*r, sizeof(Reference));
            *r = nr;
            r1 = r;
            //&fprintf(dumpOut,"! removing reference %d:%d\n", cr->position.line, cr->position.col);
        }
    }
}


static void olcxTopReferencesRemoveWindow(void) {
    OlcxReferences    *top1;
    int                 wdfile;
    Position          fp, tp;

    // in reality this is a hack, it takes references kept from
    // last file processing
    assert(currentUserData && currentUserData->browserStack.top);
    assert(currentUserData->browserStack.top->previous);
    assert(options.server_operation == OLO_REMOVE_WIN);
    wdfile = getFileNumberFromName(options.olcxWinDelFile);
    fp = makePosition(wdfile, options.olcxWinDelFromLine, options.olcxWinDelFromCol);
    tp = makePosition(wdfile, options.olcxWinDelToLine, options.olcxWinDelToCol);
    top1 = currentUserData->browserStack.top;
    olcxRemoveRefWinFromRefList(&top1->references, wdfile, &fp, &tp);
    for (SymbolsMenu *mm=top1->menuSym; mm!=NULL; mm=mm->next) {
        olcxRemoveRefWinFromRefList(&mm->s.refs, wdfile, &fp, &tp);
    }
    top1->actual = top1->references;
    fprintf(communicationChannel,"*");
}

char *getXrefEnvironmentValue(char *name ) {
    char *val = NULL;
    int n = options.setGetEnv.num;

    for (int i=0; i<n; i++) {
        //&fprintf(dumpOut,"checking (%s) %s\n",options.setGetEnv.name[i], options.setGetEnv.value[i]);
        if (strcmp(options.setGetEnv.name[i], name)==0) {
            val = options.setGetEnv.value[i];
            break;
        }
    }
    return val;
}

static void olcxProcessGetRequest(void) {
    char *name, *val;

    name = options.getValue;
    //&fprintf(dumpOut,"![get] looking for %s\n", name);
    val = getXrefEnvironmentValue(name);
    if (val != NULL) {
        // O.K. this is a special case, if input file is given
        // then make additional 'predefined' replacements
        if (options.xref2) {
            ppcGenRecord(PPC_SET_INFO, expandSpecialFilePredefinedVariables_st(val, inputFilename));
        } else {
            fprintf(communicationChannel,"*%s", expandSpecialFilePredefinedVariables_st(val, inputFilename));
        }
    } else {
        char tmpBuff[TMP_BUFF_SIZE];
        if (options.xref2) {
            sprintf(tmpBuff,"No \"-set %s <command>\" option specified for active project", name);
        } else {
            sprintf(tmpBuff,"No \"-set %s <command>\" option specified for active project", name);
        }
        errorMessage(ERR_ST, tmpBuff);
    }
}

void olcxPrintPushingAction(int opt, int afterMenu) {
    switch (opt) {
    case OLO_PUSH:
        if (olcxCheckSymbolExists()) {
            olcxOrderRefsAndGotoDefinition(afterMenu);
        } else {
            // to auto repush symbol by name, but I do not like it.
            //& if (options.xref2) ppcGenRecord(PPC_NO_SYMBOL, "");
            //& else
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_PUSH_NAME:
        if (olcxCheckSymbolExists()) {
            olcxOrderRefsAndGotoDefinition(afterMenu);
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_PUSH_SPECIAL_NAME:
        assert(0);      // called only from refactory
        break;
    case OLO_MENU_GO:
        if (olcxCheckSymbolExists()) {
            olcxOrderRefsAndGotoFirst();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_GLOBAL_UNUSED:
    case OLO_LOCAL_UNUSED:
        // no output for dead code detection ???
        break;
    case OLO_LIST:
        if (olcxCheckSymbolExists()) {
            olcxReferenceList(";");
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_USELESS_LONG_NAME:
    case OLO_USELESS_LONG_NAME_IN_CLASS:
    case OLO_PUSH_ENCAPSULATE_SAFETY_CHECK:
        olcxPushOnly();
        break;
    case OLO_PUSH_ONLY:
        if (olcxCheckSymbolExists()) {
            olcxPushOnly();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_PUSH_AND_CALL_MACRO:
        if (olcxCheckSymbolExists()) {
            olcxPushAndCallMacro();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(currentUserData->browserStack.top);
        }
        break;
    case OLO_PUSH_FOR_LOCALM:
        if (olcxCheckSymbolExists()) olcxPushOnly();
        else olcxNoSymbolFoundErrorMessage();
        break;
    case OLO_MAYBE_THIS:
    case OLO_NOT_FQT_REFS: case OLO_NOT_FQT_REFS_IN_CLASS:
    case OLO_PUSH_ALL_IN_METHOD:
        if (olcxCheckSymbolExists()) olcxPushOnly();
        else olcxNoSymbolFoundErrorMessage();
        break;
    case OLO_RENAME: case OLO_VIRTUAL2STATIC_PUSH:
    case OLO_ARG_MANIP: case OLO_ENCAPSULATE:
        if (olcxCheckSymbolExists()) olcxRenameInit();
        else olcxNoSymbolFoundErrorMessage();
        break;
    case OLO_SAFETY_CHECK2:
        if (olcxCheckSymbolExists()) olcxSafetyCheck2();
        else olcxNoSymbolFoundErrorMessage();
        break;
    default:
        assert(0);
    }
}

static void olcxCreateClassTree(void) {
    OlcxReferences    *rstack;

    olcxFreeResolutionMenu(currentUserData->classTree.tree);
    currentUserData->classTree.tree = NULL;
    olSpecialFieldCreateSelection(LINK_NAME_CLASS_TREE_ITEM, StorageMethod);
    options.ooChecksBits = (options.ooChecksBits & ~OOC_VIRTUAL_MASK);
    options.ooChecksBits |= OOC_VIRT_RELATED;
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    olCreateSelectionMenu(rstack->command);
    currentUserData->classTree.tree = rstack->menuSym;
    rstack->menuSym = NULL;
    olcxPrintClassTree(currentUserData->classTree.tree);

    // now free special references, which will never be used
    for (SymbolsMenu *ss=currentUserData->classTree.tree; ss!=NULL; ss=ss->next) {
        olcxFreeReferences(ss->s.refs);
        ss->s.refs = NULL;
    }
    // delete it as last command, because the top command is tested
    // everywhere.
    olStackDeleteSymbol(rstack);
}

void olcxPushSpecial(char *fieldName, int command) {
    int                 clii, line, col;
    OlcxReferences    *refs;
    Position          callerPos;

    clii = olSpecialFieldCreateSelection(fieldName,StorageField);
    olCreateSelectionMenu(currentUserData->browserStack.top->command);
    assert(s_javaObjectSymbol && s_javaObjectSymbol->u.structSpec);
    if (clii == s_javaObjectSymbol->u.structSpec->classFile
        || command == OLO_MAYBE_THIS
        || command == OLO_NOT_FQT_REFS
        || command == OLO_NOT_FQT_REFS_IN_CLASS
        ) {
        // object, so you have to select all
        refs = currentUserData->browserStack.top;
        for (SymbolsMenu *ss=refs->menuSym; ss!=NULL; ss=ss->next) {
            ss->visible = ss->selected = 1;
        }
        olProcessSelectedReferences(refs, genOnLineReferences);
        getLineColCursorPositionFromCommandLineOption(&line, &col);
        callerPos = makePosition(inputFileNumber, line, col);
        olSetCallerPosition(&callerPos);
    }
}

static void olcxListSpecial(char *fieldName) {
    olcxPushSpecial(fieldName, options.server_operation);
    //&olcxPrintSelectionMenu(currentUserData->browserStack.top->menuSym);
    // previous do not work, because of automatic reduction in moving
    olcxPrintPushingAction(options.server_operation, DEFAULT_VALUE);
}

bool isPushAllMethodsValidRefItem(SymbolReferenceItem *ri) {
    if (ri->name[0]!=' ')
        return true;
    if (ri->b.symType==TypeInducedError)
        return true;
    //& if (strcmp(ri->name, LINK_NAME_MAYBE_THIS_ITEM)==0) return true;
    return false;
}

static void olPushAllReferencesInBetweenMapFun(SymbolReferenceItem *ri,
                                               void *ddd
                                               ) {
    Reference             *rr, *defRef;
    Position              defpos;
    OlcxReferences        *rstack;
    SymbolsMenu         *mm;
    S_pushAllInBetweenData  *dd;
    int                     defusage,select,visible,ooBits,vlevel;

    dd = (S_pushAllInBetweenData *) ddd;
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    if (!isPushAllMethodsValidRefItem(ri)) return;
    for (rr=ri->refs; rr!=NULL; rr=rr->next) {
        log_trace("checking %d.%d ref of %s", rr->position.line,rr->position.col,ri->name);
        if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, dd)) {
            defRef = getDefinitionRef(ri->refs);
            if (defRef!=NULL && IS_DEFINITION_OR_DECL_USAGE(defRef->usage.base)) {
                defpos = defRef->position;
                defusage = defRef->usage.base;
            } else {
                defpos = noPosition;
                defusage = UsageNone;
            }
            select = visible = 1;
            vlevel = 0;
            ooBits = (OOC_PROFILE_EQUAL | OOC_VIRT_SAME_FUN_CLASS);
            log_trace("adding symbol %s", ri->name);
            mm = olAddBrowsedSymbol(ri, &rstack->menuSym, select, visible, ooBits, USAGE_ANY, vlevel, &defpos, defusage);
            assert(mm!=NULL);
            for (; rr!=NULL; rr=rr->next) {
                log_trace("checking reference of line %d, usage %s", rr->position.line, usageEnumName[rr->usage.base]);
                if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr,dd)) {
                    //& olcxAddReferenceToSymbolsMenu(mm, rr, 0);
                    log_trace("adding reference of line %d",rr->position.line);
                    olcxAddReferenceNoUsageCheck(&mm->s.refs, rr, 0);
                }
            }
            goto fini;
        }
    }
 fini:;
}

void olPushAllReferencesInBetween(int minMemi, int maxMemi) {
    S_pushAllInBetweenData  rr;
    OlcxReferences        *rstack;
    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    rr.minMemi = minMemi;
    rr.maxMemi = maxMemi;
    refTabMap2(&referenceTable, olPushAllReferencesInBetweenMapFun, &rr);
    olProcessSelectedReferences(rstack, genOnLineReferences);
    //&olcxPrintSelectionMenu(currentUserData->browserStack.top->menuSym);
}

static bool tpCheckUniquityOfSymbol(char *fieldOrMethod) {
    OlcxReferences *rstack;
    SymbolsMenu *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    if (ss == NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"No symbol selected, please position the cursor on a definition of a %s (on its name) before invoking this refactoring.", fieldOrMethod);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    if (ss->next != NULL) {
        errorMessage(ERR_ST,"Ambiguous symbol. You are probably trying to refactor a constructor or a native method. C-xrefactory does not know to handle those cases, sorry.");
        return false;
    }
    return true;
}

static bool tpCheckThatCurrentReferenceIsDefinition(char *fieldOrMethod) {
    OlcxReferences *rstack;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    if (rstack->actual==NULL
        || ! IS_DEFINITION_OR_DECL_USAGE(rstack->actual->usage.base)) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"This refactoring requires cursor (point) to be positioned at the definition of a %s (on its name). Please move to the definition first.", fieldOrMethod);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckItIsAFieldOrMethod(int require, char *fieldOrMethod) {
    OlcxReferences *rstack;
    SymbolsMenu *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    if (ss->s.b.symType!=TypeDefault
        || (ss->s.b.storage!=StorageField && require==REQ_FIELD)
        || (ss->s.b.storage!=StorageMethod && require==REQ_METHOD)) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "This does not look like a %s, please position the cursor on a definition of a %s (on its name) before invoking this refactoring.",
                fieldOrMethod, fieldOrMethod);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckStaticity(int require,char *fieldOrMethod) {
    OlcxReferences *rstack;
    SymbolsMenu *ss;
    char ttt[MAX_CX_SYMBOL_SIZE];
    char tmpBuff[TMP_BUFF_SIZE];

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    if (require==REQ_STATIC && (ss->s.b.accessFlags & AccessStatic) == 0) {
        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        sprintf(tmpBuff,"%c%s %s is not static, this refactoring requires a static %s.",
                toupper(*fieldOrMethod), fieldOrMethod+1, ttt, fieldOrMethod);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    } else if (require==REQ_NONSTATIC && (ss->s.b.accessFlags & AccessStatic)) {
        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        sprintf(tmpBuff,"%c%s %s is declared static, this refactoring requires a non static %s.",
                toupper(*fieldOrMethod), fieldOrMethod+1, ttt, fieldOrMethod);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static Symbol *javaGetClassSymbolFromClassDotName(char *fqName) {
    char *dd, *sn;
    char ttt[MAX_CX_SYMBOL_SIZE];

    strcpy(ttt, fqName);
    dd = sn = ttt;
    while ((dd=strchr(dd,'.'))!=NULL) {
        *dd = '/';
        sn = dd+1;
    }
    return javaFQTypeSymbolDefinition(sn, ttt);
}

Symbol *getMoveTargetClass(void) {
    if (options.moveTargetClass == NULL) {
        errorMessage(ERR_INTERNAL,"pull up/push down pre-check without setting target class");
        return NULL;
    }
    return javaGetClassSymbolFromClassDotName(options.moveTargetClass);
}

static bool tpCheckItIsAPackage(int req, char *classOrPack) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    if ((req==REQ_PACKAGE && ss->s.b.symType!=TypePackage) ||
        (req==REQ_CLASS && ss->s.b.symType!=TypeStruct)) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "This does not look like a %s name, please position the cursor on a %s name before invoking this refactoring.", classOrPack, classOrPack);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckPrintClassMovingType(void) {
    OlcxReferences        *rstack;
    SymbolsMenu         *ss;
    S_tpCheckMoveClassData  dd;
    char                    spack[MAX_FILE_NAME_SIZE];
    char                    tpack[MAX_FILE_NAME_SIZE];

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    if (ss->s.b.accessFlags & AccessStatic) {
        fprintf(communicationChannel,"*nested");
    } else {
        fprintf(communicationChannel,"*outer");
    }
    tpCheckFillMoveClassData(&dd, spack, tpack);
    if (dd.transPackageMove) {
        fprintf(communicationChannel, ";transpackage");
    }
    return true;
}

static bool tpCheckPrintSelectedSymbol(void) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    fprintf(communicationChannel,"*");
    printSymbolLinkName(communicationChannel, ss);
    return true;
}

int javaGetSuperClassNumFromClassNum(int cn) {
    for (ClassHierarchyReference *cl = fileTable.tab[cn]->superClasses; cl!=NULL; cl=cl->next) {
        int res = cl->superClass;
        if (! fileTable.tab[res]->b.isInterface)
            return res;
    }
    return noFileIndex;
}

bool javaIsSuperClass(int superclas, int clas) {
    int ss;
    for (ss=javaGetSuperClassNumFromClassNum(clas);
         ss!=noFileIndex && ss!=superclas;
         ss=javaGetSuperClassNumFromClassNum(ss))
        ;
    if (ss == superclas) return true;
    return false;
}

static bool tpCheckPrintPushingDownMethodType(void) {
    fprintf(communicationChannel, "*** push_down_method trivial precheck passed.");
    return true;
#if ZERO
    /* WTF? What is all this? it only returns true... Is that why it's commented out? */
    assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
    rstack = s_olcxCurrentUser->browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    srccn = ss->s.vApplClass;
    target = getMoveTargetClass();
    assert(target!=NULL && target->u.s);
    targetcn = target->u.s->classFile;
    // this is basically to print 'deletingAllowed' or not to print it
    for (mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
        if (itIsSameCxSymbol(&ss->s, &mm->s)) {
            // is there a reference to it from another subclass than target?
            if (mm->s.vFunClass == ss->s.vFunClass) {
                // rafn also avoids the original definition
                if (mm->refn!=0 && ! isSmallerOrEqClass(mm->s.vApplClass, targetcn)) {
                    fprintf(ccOut,"* multiCopyPushDown");
                    return true;
                }
            }
            // is there an application to original class or some of super types?
            // you should not consider any recursive call from within the method,
            // am I right?
            // avoid testing of super.sym() of source file
            // But HOW TO DO ALL THOSE TESTS?, maybe if done as post checks?
            if (mm->refn!=0 && isSmallerOrEqClass(srccn, mm->s.vApplClass)) {
                fprintf(ccOut,"* singleCopyPushDown");
                return true;
            }
        }
    }
    fprintf(ccOut, "* singleDeletingPushDown");
    return true;
#endif
}


static void olTrivialRefactoringPreCheck(int refcode) {
    OlcxReferences *tpchsymbol;

    olCreateSelectionMenu(currentUserData->browserStack.top->command);
    tpchsymbol = currentUserData->browserStack.top;
    switch (refcode) {
    case TPC_MOVE_FIELD:
        (void) (tpCheckUniquityOfSymbol("field") &&
                tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
                tpCheckThatCurrentReferenceIsDefinition("field") &&
                fprintf(communicationChannel,"*** move_static_field trivial precheck passed."));
        break;
    case TPC_MOVE_STATIC_FIELD:
        (void) (tpCheckUniquityOfSymbol("field") &&
                tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
                tpCheckThatCurrentReferenceIsDefinition("field") &&
                tpCheckStaticity(REQ_STATIC,"field") &&
                fprintf(communicationChannel,"*** move_static_field trivial precheck passed."));
        break;
    case TPC_MOVE_STATIC_METHOD:
        (void) (tpCheckUniquityOfSymbol("method") &&
                tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
                tpCheckThatCurrentReferenceIsDefinition("method") &&
                tpCheckStaticity(REQ_STATIC,"method") &&
                fprintf(communicationChannel,"*** move_static_method trivial precheck passed."));
        break;
    case TPC_TURN_STATIC_METHOD_TO_DYN:
        (void) (tpCheckUniquityOfSymbol("method") &&
                tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
                tpCheckThatCurrentReferenceIsDefinition("method") &&
                tpCheckStaticity(REQ_STATIC,"method") &&
                fprintf(communicationChannel,"*** turn_static_method_to_dyn trivial precheck passed."));
        break;
    case TPC_TURN_DYN_METHOD_TO_STATIC:
        (void) (tpCheckUniquityOfSymbol("method") &&
                tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
                tpCheckThatCurrentReferenceIsDefinition("method") &&
                tpCheckStaticity(REQ_NONSTATIC,"method") &&
                tpCheckOuterScopeUsagesForDynToSt() &&
                tpCheckSuperMethodReferencesForDynToSt() &&
                fprintf(communicationChannel,"*** turn_static_method_to_dyn trivial precheck passed."));
        break;
    case TPC_PULL_UP_METHOD:
        (void) (tpCheckUniquityOfSymbol("method") &&
                tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
                tpCheckThatCurrentReferenceIsDefinition("method") &&
                tpCheckStaticity(REQ_NONSTATIC,"method") &&
                tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS,"superclass") &&
                tpCheckSuperMethodReferencesForPullUp() &&
                tpCheckMethodReferencesWithApplOnSuperClassForPullUp() &&
                fprintf(communicationChannel,"*** pull_up_method trivial precheck passed."));
        break;
    case TPC_PUSH_DOWN_METHOD:
        (void) (tpCheckUniquityOfSymbol("method") &&
                tpCheckItIsAFieldOrMethod(REQ_METHOD, "method") &&
                tpCheckThatCurrentReferenceIsDefinition("method") &&
                tpCheckStaticity(REQ_NONSTATIC,"method") &&
                tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass") &&
                tpCheckPrintPushingDownMethodType());
                break;
    case TPC_PUSH_DOWN_METHOD_POST_CHECK:
        (void) (tpCheckSuperMethodReferencesAfterPushDown() &&
                fprintf(communicationChannel,"*** push_down_method trivial postcheck passed."));
        break;
    case TPC_PULL_UP_FIELD:
        (void) (tpCheckUniquityOfSymbol("field") &&
                tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
                tpCheckThatCurrentReferenceIsDefinition("field") &&
                tpCheckStaticity(REQ_NONSTATIC,"field") &&
                tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS,"superclass") &&
                tpPullUpFieldLastPreconditions() &&
                fprintf(communicationChannel,"*** pull_up_field trivial precheck passed."));
        break;
    case TPC_PUSH_DOWN_FIELD:
        (void) (tpCheckUniquityOfSymbol("field") &&
                tpCheckItIsAFieldOrMethod(REQ_FIELD, "field") &&
                tpCheckThatCurrentReferenceIsDefinition("field") &&
                tpCheckStaticity(REQ_NONSTATIC,"field") &&
                tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass") &&
                tpPushDownFieldLastPreconditions() &&
                fprintf(communicationChannel,"*** push_down_field trivial precheck passed."));
        break;
    case TPC_RENAME_PACKAGE:
        (void) (tpCheckUniquityOfSymbol("package") &&
                tpCheckItIsAPackage(REQ_PACKAGE, "package") &&
                tpCheckPrintSelectedSymbol());
        break;
    case TPC_RENAME_CLASS:
        (void) (tpCheckUniquityOfSymbol("class") &&
                tpCheckItIsAPackage(REQ_CLASS, "class") &&
                tpCheckPrintSelectedSymbol());
        break;
    case TPC_MOVE_CLASS:
        (void) (tpCheckUniquityOfSymbol("class") &&
                tpCheckItIsAPackage(REQ_CLASS, "class") &&
                tpCheckSourceIsNotInnerClass() &&
                tpCheckMoveClassAccessibilities() &&
                tpCheckPrintClassMovingType());
        break;
    case TPC_GET_LAST_IMPORT_LINE:
        if (options.xref2) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "%d", s_cps.lastImportLine);
            ppcGenRecord(PPC_SET_INFO, tmpBuff);
        } else {
            fprintf(communicationChannel,"*%d", s_cps.lastImportLine);
        }
        break;
    default:
        errorMessage(ERR_INTERNAL,"trivial precheck called with no valid check code");
    }
    // remove the checked symbol from stack
    olStackDeleteSymbol(tpchsymbol);
}

static void mainAnswerReferencePushingAction(int command) {
    assert(creatingOlcxRefs());
    //&olcxPrintSelectionMenu(currentUserData->browserStack.top->hkSelectedSym);
    //&olcxPrintSelectionMenu(currentUserData->browserStack.top->hkSelectedSym);
    olCreateSelectionMenu(command);
    //&olcxPrintSelectionMenu(currentUserData->browserStack.top->hkSelectedSym);
    //&olcxDumpSelectionMenu(currentUserData->browserStack.top->menuSym);
    if (options.manualResolve == RESOLVE_DIALOG_ALLWAYS
        || safetyCheck2ShouldWarn()
        || (olcxShowSelectionMenu()
            && options.manualResolve != RESOLVE_DIALOG_NEVER)) {
        if (options.xref2) {
            ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
        } else {
            olcxPrintSelectionMenu(currentUserData->browserStack.top->menuSym);
        }
    } else {
        assert(currentUserData && currentUserData->browserStack.top);
        //&olProcessSelectedReferences(currentUserData->browserStack.top, genOnLineReferences);
        olcxPrintPushingAction(options.server_operation, DEFAULT_VALUE);
    }
}

static void mapAddLocalUnusedSymbolsToHkSelection(SymbolReferenceItem *ss) {
    bool used = false;
    Reference *definitionReference = NULL;

    if (ss->b.category != CategoryLocal)
        return;
    for (Reference *r = ss->refs; r!=NULL; r=r->next) {
        if (IS_DEFINITION_OR_DECL_USAGE(r->usage.base)) {
            if (r->position.file == inputFileNumber) {
                if (IS_DEFINITION_USAGE(r->usage.base)) {
                    definitionReference = r;
                }
                if (definitionReference == NULL)
                    definitionReference = r;
            }
        } else {
            used = true;
            break;
        }
    }
    if (!used && definitionReference!=NULL) {
        olAddBrowsedSymbol(ss,&currentUserData->browserStack.top->hkSelectedSym,
                           1,1,0,UsageDefined,0, &definitionReference->position, definitionReference->usage.base);
    }
}

void pushLocalUnusedSymbolsAction(void) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss == NULL);
    refTabMap(&referenceTable, mapAddLocalUnusedSymbolsToHkSelection);
    olCreateSelectionMenu(options.server_operation);
}

static void answerPushLocalUnusedSymbolsAction(void) {
    pushLocalUnusedSymbolsAction();
    assert(options.xref2);
    ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
}

static void answerPushGlobalUnusedSymbolsAction(void) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(currentUserData && currentUserData->browserStack.top);
    rstack = currentUserData->browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss == NULL);
    scanReferenceFiles(options.cxrefFileName, deadCodeDetectionFunctionSequence);
    olCreateSelectionMenu(options.server_operation);
    assert(options.xref2);
    ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
}

static void getCallerPositionFromCommandLineOption(Position *opos) {
    int file, line, col;

    assert(opos != NULL);
    file = olOriginalFileNumber;
    getLineColCursorPositionFromCommandLineOption(&line, &col);
    *opos = makePosition(file, line, col);
}

static void answerClassName(char *name) {
    char ttt[MAX_CX_SYMBOL_SIZE];
    if (*name!=0) {
        linkNamePrettyPrint(ttt, name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        if (options.xref2) {
            ppcGenRecord(PPC_SET_INFO, ttt);
        } else {
            fprintf(communicationChannel,"*%s", ttt);
        }
    } else {
        errorMessage(ERR_ST, "Not inside a class. Can't get current class.");
    }
}

static void pushSymbolByName(char *name) {
    OlcxReferences *rstack;
    if (s_cache.cpi>0) {
        int spass;
        spass = currentPass; currentPass=1;
        recoverCachePointZero();
        currentPass = spass;
    }
    rstack = currentUserData->browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(name, noFileIndex, StorageDefault);
    getCallerPositionFromCommandLineOption(&rstack->callerPosition);
}

void mainAnswerEditAction(void) {
    OlcxReferences *rstack, *nextrr;
    Position opos;
    char *ifname, *jdkcp;
    char dffname[MAX_FILE_NAME_SIZE];
    char dffsect[MAX_FILE_NAME_SIZE];

    ENTER();
    assert(communicationChannel);

    log_trace("Server operation = %s(%d)", operationNamesTable[options.server_operation], options.server_operation);
    switch (options.server_operation) {
    case OLO_CHECK_VERSION:
        assert(options.checkVersion!=NULL);
        if (strcmp(options.checkVersion, C_XREF_VERSION_NUMBER)!=0) {
            ppcGenRecord(PPC_VERSION_MISMATCH, C_XREF_VERSION_NUMBER);
        }
        break;
    case OLO_COMPLETION:
    case OLO_SEARCH:
        printCompletions(&s_completions);
        break;
    case OLO_EXTRACT:
        if (! s_cps.extractProcessedFlag) {
            fprintf(communicationChannel,"*** No function/method enclosing selected block found **");
        }
        break;
    case OLO_TAG_SEARCH:
        getCallerPositionFromCommandLineOption(&opos);
        //&olCompletionListInit(&opos);
        if (! options.xref2) fprintf(communicationChannel,";");
        assert(currentUserData);
        olcxPushEmptyStackItem(&currentUserData->retrieverStack);
        currentUserData->retrieverStack.top->callerPosition = opos;

        s_wildcardSearch = containsWildcard(options.olcxSearchString);

        if (options.tagSearchSpecif==TSS_FULL_SEARCH) scanJarFilesForTagSearch();
        scanReferenceFiles(options.cxrefFileName, symbolSearchFunctionSequence);
        printTagSearchResults();
        break;
    case OLO_TAG_SEARCH_BACK:
        olcxSetCurrentUser(options.user);
        if (currentUserData->retrieverStack.top!=NULL &&
            currentUserData->retrieverStack.top->previous!=NULL) {
            currentUserData->retrieverStack.top = currentUserData->retrieverStack.top->previous;
            ppcGotoPosition(&currentUserData->retrieverStack.top->callerPosition);
            printTagSearchResults();
        }
        break;
    case OLO_TAG_SEARCH_FORWARD:
        olcxSetCurrentUser(options.user);
        nextrr = getNextTopStackItem(&currentUserData->retrieverStack);
        if (nextrr != NULL) {
            currentUserData->retrieverStack.top = nextrr;
            ppcGotoPosition(&currentUserData->retrieverStack.top->callerPosition);
            printTagSearchResults();
        }
        break;
    case OLO_ACTIVE_PROJECT:
        if (options.project != NULL) {
            if (options.xref2) {
                ppcGenRecord(PPC_SET_INFO, options.project);
            } else {
                fprintf(communicationChannel,"*%s", options.project);
            }
        } else {
            if (olOriginalComFileNumber == noFileIndex) {
                if (options.xref2) {
                    ppcGenRecord(PPC_ERROR, "No source file to identify project");
                } else {
                    fprintf(communicationChannel,"!** No source file to identify project");
                }
            } else {
                ifname = fileTable.tab[olOriginalComFileNumber]->name;
                log_trace("ifname = %s", ifname);
                searchDefaultOptionsFile(ifname, dffname, dffsect);
                if (dffname[0]==0 || dffsect[0]==0) {
                    if (options.noErrors) {
                        if (!options.xref2)
                            fprintf(communicationChannel,"^"); // TODO: was "fprintf(ccOut,"^", ifname);"
                    } else {
                        if (options.xref2) {
                            ppcGenRecord(PPC_NO_PROJECT, ifname);
                        } else {
                            fprintf(communicationChannel,"!** No project name matches %s", ifname);
                        }
                    }
                } else {
                    if (options.xref2) {
                        ppcGenRecord(PPC_SET_INFO, dffsect);
                    } else {
                        fprintf(communicationChannel,"*%s", dffsect);
                    }
                }
            }
        }
        break;
    case OLO_JAVA_HOME:
        jdkcp = getJavaHome();
        if (options.xref2) {
            if (jdkcp==NULL) {
                if (! options.noErrors) {
                    ppcGenRecord(PPC_ERROR, "Can't infer Java home");
                }
            } else {
                ppcGenRecord(PPC_SET_INFO, jdkcp);
            }
        } else {
            if (jdkcp==NULL) {
                if (options.noErrors) {
                    fprintf(communicationChannel,"^");
                } else {
                    fprintf(communicationChannel,"!* Can't find Java runtime library rt.jar"); // TODO: was "..., ifname);"
                }
            } else {
                fprintf(communicationChannel,"*%s", jdkcp);
            }
        }
        break;
    case OLO_GET_ENV_VALUE:
        olcxProcessGetRequest();
        break;
    case OLO_NEXT:
        olcxReferencePlus();
        break;
    case OLO_PREVIOUS:
        olcxReferenceMinus();
        break;
    case OLO_GOTO_DEF:
        olcxReferenceGotoDef();
        break;
    case OLO_GOTO_CALLER:
        olcxReferenceGotoCaller();
        break;
    case OLO_GOTO_CURRENT:
        olcxReferenceGotoCurrent();
        break;
    case OLO_GET_CURRENT_REFNUM:
        olcxReferenceGetCurrentRefn();
        break;
    case OLO_LIST_TOP:
        olcxListTopReferences(";");
        break;
    case OLO_SHOW_TOP:
        olcxShowTopSymbol();
        break;
    case OLO_SHOW_TOP_APPL_CLASS:
        olcxShowTopApplClass();
        break;
    case OLO_SHOW_TOP_TYPE:
        olcxShowTopType();
        break;
    case OLO_SHOW_CLASS_TREE:
        olcxShowClassTree();
        break;
    case OLO_TOP_SYMBOL_RES:
        olcxTopSymbolResolution();
        break;
    case OLO_CSELECT:
        olCompletionSelect();
        break;
    case OLO_COMPLETION_BACK:
        olCompletionBack();
        break;
    case OLO_COMPLETION_FORWARD:
        olCompletionForward();
        break;
    case OLO_GOTO:
        olcxReferenceGotoRef(options.olcxGotoVal);
        break;
    case OLO_CGOTO:
        olcxReferenceGotoCompletion(options.olcxGotoVal);
        break;
    case OLO_TAGGOTO:
        olcxReferenceGotoTagSearchItem(options.olcxGotoVal);
        break;
    case OLO_TAGSELECT:
        olcxReferenceSelectTagSearchItem(options.olcxGotoVal);
        break;
    case OLO_CBROWSE:
        olcxReferenceBrowseCompletion(options.olcxGotoVal);
        break;
    case OLO_REF_FILTER_SET:
        olcxReferenceFilterSet(options.filterValue);
        break;
    case OLO_REPUSH:
        olcxReferenceRePush();
        break;
    case OLO_POP:
        olcxReferencePop();
        break;
    case OLO_POP_ONLY:
        olcxPopOnly();
        break;
    case OLO_MENU_INSPECT_DEF:
        olcxSymbolMenuInspectDef();
        break;
    case OLO_MENU_INSPECT_CLASS:
        olcxSymbolMenuInspectClass();
        break;
    case OLO_MENU_SELECT:
        olcxMenuToggleSelect();
        break;
    case OLO_MENU_SELECT_ONLY:
        olcxMenuSelectOnly();
        break;
    case OLO_MENU_SELECT_ALL:
        olcxMenuSelectAll(1);
        break;
    case OLO_MENU_SELECT_NONE:
        olcxMenuSelectAll(0);
        break;
    case OLO_MENU_FILTER_SET:
        olcxMenuSelectPlusolcxMenuSelectFilterSet(options.filterValue);
        break;
    case OLO_SAFETY_CHECK_INIT:
        olcxSafetyCheckInit();
        break;
    case OLO_SAFETY_CHECK1:
        olcxSafetyCheck1();
        break;
    case OLO_MM_PRE_CHECK:
    case OLO_PP_PRE_CHECK:
        olcxMMPreCheck();   // the value of s_opt.cxrefs is checked inside
        break;
    case OLO_INTERSECTION:
        olcxTopReferencesIntersection();
        break;
    case OLO_REMOVE_WIN:
        olcxTopReferencesRemoveWindow();
        break;
    case OLO_MENU_GO:
        assert(currentUserData && currentUserData->browserStack.top);
        rstack = currentUserData->browserStack.top;
        //&olProcessSelectedReferences(rstack, genOnLineReferences);
        olcxPrintPushingAction(currentUserData->browserStack.top->command,
                               PUSH_AFTER_MENU);
        break;
    case OLO_CT_INSPECT_DEF:
        olcxClassTreeInspectDef();
        break;
    case OLO_CLASS_TREE:
        olcxCreateClassTree();
        break;
    case OLO_USELESS_LONG_NAME:
    case OLO_USELESS_LONG_NAME_IN_CLASS:
        olcxListSpecial(LINK_NAME_IMPORTED_QUALIFIED_ITEM);
        break;
    case OLO_MAYBE_THIS:
        olcxListSpecial(LINK_NAME_MAYBE_THIS_ITEM);
        break;
    case OLO_NOT_FQT_REFS:
    case OLO_NOT_FQT_REFS_IN_CLASS:
        olcxListSpecial(LINK_NAME_NOT_FQT_ITEM);
        break;
    case OLO_SET_MOVE_TARGET:
        // xref1 target setting
        assert(!options.xref2);
        if (*s_cps.setTargetAnswerClass!=0) {
            fprintf(communicationChannel,"*");
            //&printSymbolLinkNameString(communicationChannel, s_cps.setTargetAnswerClass);
            // very risky, will need a lot of adjustements in xref.el
            fprintf(communicationChannel, "%s", s_cps.setTargetAnswerClass);
        } else {
            errorMessage(ERR_ST, "Not a valid target position. The cursor has to be on a place where a new field/method can be inserted.");
        }
        break;
    case OLO_SET_MOVE_CLASS_TARGET:
    case OLO_SET_MOVE_METHOD_TARGET:
        // xref2 target
        assert(options.xref2);
        if (!s_cps.moveTargetApproved) {
            ppcGenRecord(PPC_ERROR, "Invalid target place");
        }
        break;
    case OLO_GET_CURRENT_CLASS:
        answerClassName(s_cps.currentClassAnswer);
        break;
    case OLO_GET_CURRENT_SUPER_CLASS:
        answerClassName(s_cps.setTargetAnswerClass);
        break;
    case OLO_GET_METHOD_COORD:
        if (s_cps.methodCoordEndLine!=0) {
            fprintf(communicationChannel,"*%d", s_cps.methodCoordEndLine);
        } else {
            errorMessage(ERR_ST, "No method found.");
        }
        break;
    case OLO_GET_CLASS_COORD:
        if (s_cps.classCoordEndLine!=0) {
            fprintf(communicationChannel,"*%d", s_cps.classCoordEndLine);
        } else {
            errorMessage(ERR_ST, "No class found.");
        }
        break;
    case OLO_GET_SYMBOL_TYPE:
        if (s_olstringServed) {
            fprintf(communicationChannel,"*%s", s_olSymbolType);
        } else if (options.noErrors) {
            fprintf(communicationChannel,"*");
        } else {
            errorMessage(ERR_ST, "No symbol found.");
        }
        olStackDeleteSymbol(currentUserData->browserStack.top);
        break;
    case OLO_GOTO_PARAM_NAME:
        // I hope this is not used anymore, put there assert(0);
        if (s_olstringServed && s_paramPosition.file != noFileIndex) {
            gotoOnlineCxref(&s_paramPosition, UsageDefined, "");
            olStackDeleteSymbol(currentUserData->browserStack.top);
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "Parameter %d not found.", options.olcxGotoVal);
            errorMessage(ERR_ST, tmpBuff);
        }
        break;
    case OLO_GET_PRIMARY_START:
        if (s_olstringServed && s_primaryStartPosition.file != noFileIndex) {
            gotoOnlineCxref(&s_primaryStartPosition, UsageDefined, "");
            olStackDeleteSymbol(currentUserData->browserStack.top);
        } else {
            errorMessage(ERR_ST, "Begin of primary expression not found.");
        }
        break;
    case OLO_PUSH_ALL_IN_METHOD:
        //&fprintf(communicationChannel,":\n\n getting all references from %d to %d\n", s_cps.cxMemoryIndexAtMethodBegin, s_cps.cxMemoryIndexAtMethodEnd);
        olPushAllReferencesInBetween(s_cps.cxMemoryIndexAtMethodBegin, s_cps.cxMemoryIndexAtMethodEnd);
        olcxPrintPushingAction(options.server_operation, 0);
        break;
    case OLO_TRIVIAL_PRECHECK:
        olTrivialRefactoringPreCheck(options.trivialPreCheckCode);
        break;
    case OLO_ENCAPSULATE_SAFETY_CHECK:
        olEncapsulationSafetyCheck();
        break;
    case OLO_GET_AVAILABLE_REFACTORINGS:
        olGetAvailableRefactorings();
        olStackDeleteSymbol(currentUserData->browserStack.top);
        break;
    case OLO_PUSH_NAME:
        pushSymbolByName(options.pushName);
        mainAnswerReferencePushingAction(options.server_operation);
        break;
    case OLO_GLOBAL_UNUSED:
        answerPushGlobalUnusedSymbolsAction();
        break;
    case OLO_LOCAL_UNUSED:
        answerPushLocalUnusedSymbolsAction();
        break;
    case OLO_PUSH_SPECIAL_NAME:
        assert(0);  // only refactory
        break;
    case OLO_ARG_MANIP:
        rstack = currentUserData->browserStack.top;
        assert(rstack!=NULL);
        if (rstack->hkSelectedSym == NULL ||
            (LANGUAGE(LANG_JAVA) &&
             rstack->hkSelectedSym->s.b.storage!=StorageMethod &&
             rstack->hkSelectedSym->s.b.storage!=StorageConstructor)) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"Cursor (point) has to be positioned on a method or constructor name before invocation of this refactoring, not on the parameter itself. Please move the cursor onto the method (constructor) name and reinvoke the refactoring.");
            errorMessage(ERR_ST, tmpBuff);
        } else {
            mainAnswerReferencePushingAction(options.server_operation);
        }
        break;
    default:
        mainAnswerReferencePushingAction(options.server_operation);
    } // switch

    fflush(communicationChannel);
    inputFilename = NULL;
    //&RLM_FREE_COUNT(olcxMemory);
    LEAVE();
}

int itIsSymbolToPushOlReferences(SymbolReferenceItem *p,
                               OlcxReferences *rstack,
                               SymbolsMenu **rss,
                               int checkSelFlag) {
    for (SymbolsMenu *ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
        if ((ss->selected || checkSelFlag==DO_NOT_CHECK_IF_SELECTED)
            && ss->s.vApplClass == p->vApplClass
            && ss->s.vFunClass == p->vFunClass
            && isSameCxSymbol(p, &ss->s)) {
            *rss = ss;
            if (IS_BEST_FIT_MATCH(ss)) {
                return 2;
            } else {
                return 1;
            }
        }
    }
    *rss = NULL;
    return 0;
}


void putOnLineLoadedReferences(SymbolReferenceItem *p) {
    int ols;
    SymbolsMenu *cms;

    ols = itIsSymbolToPushOlReferences(p,currentUserData->browserStack.top,
                                       &cms, DO_NOT_CHECK_IF_SELECTED);
    if (ols > 0) {
        assert(cms);
        //&LIST_LEN(nn, Reference, p->refs);sprintf(tmpBuff,"!putting %d references of %s\n", nn, fileTable.tab[p->vApplClass]->name);ppcBottomInformation(tmpBuff);
        for (Reference *rr=p->refs; rr!=NULL; rr=rr->next) {
            olcxAddReferenceToSymbolsMenu(cms, rr, (ols == 2));
        }
        //&LIST_LEN(nn, Reference, cms->s.refs);sprintf(tmpBuff,"added %d refs for %s", nn, fileTable.tab[cms->s.vApplClass]->name);ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff);
    }
}

void genOnLineReferences(OlcxReferences *rstack, SymbolsMenu *cms) {
    if (cms->selected) {
        assert(cms);
        olcxAddReferences(cms->s.refs, &rstack->references, ANY_FILE,
                          IS_BEST_FIT_MATCH(cms));
    }
}

static int classCmp(int cl1, int cl2) {
    int res;
    //&fprintf(dumpOut,"classCMP %s <-> %s\n", fileTable.tab[cl1]->name, fileTable.tab[cl2]->name);
    res = isSmallerOrEqClassR(cl2, cl1, 1);
    if (res == 0) {
        res = - isSmallerOrEqClassR(cl1, cl2, 1);
    }
    //&fprintf(dumpOut,"result is %d\n",res);
    return res;
}

static unsigned olcxOoBits(SymbolsMenu *ols, SymbolReferenceItem *p) {
    unsigned ooBits;
    int olusage,vFunCl,olvFunCl,vApplCl,olvApplCl;

    ooBits = 0;
    assert(olcxIsSameCxSymbol(&ols->s, p));
    olvFunCl = ols->s.vFunClass;
    olvApplCl = ols->s.vApplClass;
    olusage = ols->olUsage;
    vFunCl = p->vFunClass;
    vApplCl = p->vApplClass;
    if (ols->s.b.symType!=TypeCppCollate) {
        if (ols->s.b.symType != p->b.symType) goto fini;
        if (ols->s.b.storage != p->b.storage) goto fini;
        if (ols->s.b.category != p->b.category)  goto fini;
    }
    if (strcmp(ols->s.name,p->name)==0) {
        ooBits |= OOC_PROFILE_EQUAL;
    }
    if (LANGUAGE(LANG_C) || LANGUAGE(LANG_YACC)
        || JAVA_STATICALLY_LINKED(ols->s.b.storage, ols->s.b.accessFlags)) {
        if (vFunCl == olvFunCl) ooBits |= OOC_VIRT_SAME_APPL_FUN_CLASS;
    } else {
        // the following may be too strong, maybe only test FunCl ???
        //     } else if (vFunCl==olvFunCl) {
        //         ooBits |= OOC_VIRT_SAME_FUN_CLASS;
        if (vApplCl == olvFunCl && vFunCl==olvFunCl) {
            ooBits |= OOC_VIRT_SAME_APPL_FUN_CLASS;
        } else if (olcxVirtualyUsageAdequate(vApplCl, vFunCl, olusage, olvApplCl, olvFunCl)) {
            ooBits |= OOC_VIRT_APPLICABLE;
        } else if (vFunCl==olvFunCl) {
            ooBits |= OOC_VIRT_SAME_FUN_CLASS;
        } else if (isSmallerOrEqClass(olvApplCl, vApplCl)
                   || isSmallerOrEqClass(vApplCl, olvApplCl)) {
            ooBits |= OOC_VIRT_RELATED;
        }
    }
 fini:
    return ooBits;
}

static unsigned ooBitsMax(unsigned oo1, unsigned oo2) {
    unsigned res;
    res = 0;
    if ((oo1&OOC_PROFILE_MASK) > (oo2&OOC_PROFILE_MASK)) {
        res |= (oo1&OOC_PROFILE_MASK);
    } else {
        res |= (oo2&OOC_PROFILE_MASK);
    }
    if ((oo1&OOC_VIRTUAL_MASK) > (oo2&OOC_VIRTUAL_MASK)) {
        res |= (oo1&OOC_VIRTUAL_MASK);
    } else {
        res |= (oo2&OOC_VIRTUAL_MASK);
    }
    return res;
}

SymbolsMenu *createSelectionMenu(SymbolReferenceItem *p) {
    OlcxReferences *rstack;
    Position *defpos;
    unsigned ooBits, oo;
    bool flag = false;
    int select,visible,vlev,vlevel,defusage;
    SymbolsMenu *res = NULL;

    rstack = currentUserData->browserStack.top;
    ooBits = 0; vlevel = 0;
    defpos = &noPosition; defusage = UsageNone;
    log_trace("ooBits for '%s'", fileTable.tab[p->vApplClass]->name);

    for (SymbolsMenu *ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
        if (olcxIsSameCxSymbol(p, &ss->s)) {
            flag = true;
            oo = olcxOoBits(ss, p);
            ooBits = ooBitsMax(oo, ooBits);
            if (defpos->file == noFileIndex) {
                defpos = &ss->defpos;
                defusage = ss->defUsage;
                //&fprintf(dumpOut,": propagating defpos (line %d) to menusym\n", defpos->line);
            }
            if (LANGUAGE(LANG_JAVA)) {
                vlev = classCmp(ss->s.vApplClass, p->vApplClass);
            } else {
                vlev = 0;
            }
            if (vlevel==0 || ABS(vlevel)>ABS(vlev)) vlevel = vlev;
            //&fprintf(dumpOut,"ooBits for %s <-> %s %o %o\n", fileTable.tab[ss->s.vApplClass]->name, p->name, oo, ooBits);
            // following line is a big hack to speed up class tree   !!!!!
            // and unused symbols displaying, hope it will work fine !!!!!
            //&if (options.server_operation == OLO_GLOBAL_UNUSED || options.server_operation==OLO_CLASS_TREE) break;
        }
    }
    if (flag) {
        select = visible = 0;  // for debug would be better 1 !
        res = olAddBrowsedSymbol(p, &rstack->menuSym, select, visible, ooBits, USAGE_ANY, vlevel, defpos, defusage);
    }
    return res;
}

void mapCreateSelectionMenu(SymbolReferenceItem *p) {
    createSelectionMenu(p);
}


/* ********************************************************************** */

static S_olCompletion *newOlCompletion(char *name,
                                       char *fullName,
                                       char *vclass,
                                       short int jindent,
                                       short int lineCount,
                                       char category,
                                       char csymType,
                                       struct reference ref,
                                       struct symbolReferenceItem sym
) {
    S_olCompletion *olCompletion = olcx_alloc(sizeof(S_olCompletion));

    olCompletion->name = name;
    olCompletion->fullName = fullName;
    olCompletion->vclass = vclass;
    olCompletion->jindent = jindent;
    olCompletion->lineCount = lineCount;
    olCompletion->category = category;
    olCompletion->csymType = csymType;
    olCompletion->ref = ref;
    olCompletion->sym = sym;
    olCompletion->next = NULL;

    return olCompletion;
}

void olSetCallerPosition(Position *pos) {
    assert(currentUserData && currentUserData->browserStack.top);
    currentUserData->browserStack.top->callerPosition = *pos;
}

// if s==NULL, then the pos is taken as default position of this ref !!!

S_olCompletion *olCompletionListPrepend(char *name, char *fullText, char *vclass, int jindent, Symbol *s,
                                        SymbolReferenceItem *rr, Reference *dfref, int cType, int vFunClass,
                                        OlcxReferences *stack) {
    S_olCompletion *cc;
    char *ss,*nn, *fullnn, *vclnn;
    int category, scope, storage, slen, nlen;
    SymbolReferenceItem sri;
    Reference reference;

    nlen = strlen(name);
    nn = olcx_memory_allocc(nlen+1, sizeof(char));
    strcpy(nn, name);
    fullnn = NULL;
    if (fullText!=NULL) {
        fullnn = olcx_memory_allocc(strlen(fullText)+1, sizeof(char));
        strcpy(fullnn, fullText);
    }
    vclnn = NULL;
    if (vclass!=NULL) {
        vclnn = olcx_memory_allocc(strlen(vclass)+1, sizeof(char));
        strcpy(vclnn, vclass);
    }
    if (rr!=NULL) {
        // probably a 'search in tag' file item
        slen = strlen(rr->name);
        ss = olcx_memory_allocc(slen+1, sizeof(char));
        strcpy(ss, rr->name);
        fillSymbolRefItem(&sri, ss, cxFileHashNumber(ss),
                                    rr->vApplClass, rr->vFunClass);
        sri.b = rr->b;

        cc = newOlCompletion(nn, fullnn, vclnn, jindent, 1, rr->b.category, cType, *dfref, sri);
    } else if (s==NULL) {
        reference = *dfref;
        reference.next = NULL;
        fillSymbolRefItem(&sri, "", cxFileHashNumber(""),
                                    noFileIndex, noFileIndex);
        fillSymbolRefItemBits(&sri.b, TypeUnknown, StorageNone,
                               ScopeAuto, AccessDefault, CategoryLocal);
        cc = newOlCompletion(nn, fullnn, vclnn, jindent, 1, CategoryLocal, cType, reference, sri);
    } else {
        getSymbolCxrefCategories(s, &category, &scope, &storage);
        log_trace(":adding sym '%s' %d", s->linkName, category);
        slen = strlen(s->linkName);
        ss = olcx_memory_allocc(slen+1, sizeof(char));
        strcpy(ss, s->linkName);
        fillUsageBits(&reference.usage, UsageDefined, 0);
        fillReference(&reference, reference.usage, s->pos, NULL);
        fillSymbolRefItem(&sri, ss, cxFileHashNumber(ss),
                                    vFunClass, vFunClass);
        fillSymbolRefItemBits(&sri.b, s->bits.symbolType, storage,
                               scope, s->bits.access, category);
        cc = newOlCompletion(nn, fullnn, vclnn, jindent, 1, category, cType, reference, sri);
    }
    if (fullText!=NULL) {
        for (int i=0; fullText[i]; i++) {
            if (fullText[i] == '\n')
                cc->lineCount++;
        }
    }
    cc->next = stack->completions;
    stack->completions = cc;
    return cc;
}

void olCompletionListReverse(void) {
    LIST_REVERSE(S_olCompletion, currentUserData->completionsStack.top->completions);
}

static int olTagSearchSortFunction(S_olCompletion *c1, S_olCompletion *c2) {
    return strcmp(c1->name, c2->name) < 0;
}

static void tagSearchShortRemoveMultipleLines(S_olCompletion *list) {
    for (S_olCompletion *l=list; l!=NULL; l=l->next) {
    again:
        if (l->next!=NULL && strcmp(l->name, l->next->name)==0) {
            // O.K. remove redundant one
            S_olCompletion *tmp = l->next;
            l->next = l->next->next;
            olcxFreeCompletion(tmp);
            goto again;          /* Again, but don't advance */
        }
    }
}

void tagSearchCompactShortResults(void) {
    assert(currentUserData);
    LIST_MERGE_SORT(S_olCompletion, currentUserData->retrieverStack.top->completions, olTagSearchSortFunction);
    if (options.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT
        || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
        tagSearchShortRemoveMultipleLines(currentUserData->retrieverStack.top->completions);
    }
}

void printTagSearchResults(void) {
    int len1, len2, len3, len;
    char *ls;

    len1 = len2 = len3 = 0;
    tagSearchCompactShortResults();

    // the first loop is counting the length of fields
    assert(currentUserData->retrieverStack.top);
    for (S_olCompletion *cc=currentUserData->retrieverStack.top->completions; cc!=NULL; cc=cc->next) {
        ls = crTagSearchLineStatic(cc->name, &cc->ref.position,
                                   &len1, &len2, &len3);
    }
    if (options.olineLen >= 50000) {
        /* TODO: WTF? 50k??!?! */
        if (len1 > MAX_TAG_SEARCH_INDENT)
            len1 = MAX_TAG_SEARCH_INDENT;
    } else {
        if (len1 > (options.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100) {
            len1 = (options.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100;
        }
    }
    len = len1;

    // the second is writing
    if (options.xref2)
        ppcBegin(PPC_SYMBOL_LIST);
    assert(currentUserData->retrieverStack.top);
    for (S_olCompletion *cc=currentUserData->retrieverStack.top->completions; cc!=NULL; cc=cc->next) {
        ls = crTagSearchLineStatic(cc->name, &cc->ref.position,
                                   &len1, &len2, &len3);
        if (options.xref2) {
            ppcGenRecord(PPC_STRING_VALUE, ls);
        } else {
            fprintf(communicationChannel,"%s\n", ls);
        }
        len1 = len;
    }
    if (options.xref2)
        ppcEnd(PPC_SYMBOL_LIST);
}
