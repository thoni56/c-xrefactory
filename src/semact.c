#include "semact.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "parsers.h"
#include "misc.h"
#include "classcaster.h"
#include "usage.h"
#include "yylex.h"
#include "cxref.h"
#include "jsemact.h"
#include "jslsemact.h"          /*& s_jsl &*/
#include "symbol.h"
#include "list.h"
#include "filetable.h"

#include "hash.h"
#include "log.h"


void fillRecFindStr(S_recFindStr *recFindStr, Symbol *baseClass, Symbol *currentClass, Symbol *nextRecord, unsigned recsClassCounter) {
    recFindStr->baseClass = baseClass;
    recFindStr->currClass = currentClass;
    recFindStr->nextRecord = nextRecord;
    recFindStr->recsClassCounter = recsClassCounter;
    recFindStr->sti = 0;
    recFindStr->aui = 0;
}

bool displayingErrorMessages(void) {
    // no error messages for Java file preloaded for symbols
    if (LANGUAGE(LANG_JAVA) && s_jsl!=NULL)
        return false;
    if (options.debug || options.errors)
        return true;
    return false;
}

int styyerror(char *s) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (strcmp(s, "syntax error") != 0) {
        sprintf(tmpBuff,"YACC error: %s", s);
        errorMessage(ERR_INTERNAL, tmpBuff);
    } else if (displayingErrorMessages()) {
        log_trace("Syntax error on: '%s'", yytext);
        sprintf(tmpBuff, "Syntax error on: %s", yytext);
        errorMessage(ERR_ST, tmpBuff);
    }
    return 0;
}

void noSuchFieldError(char *rec) {
    char message[TMP_BUFF_SIZE];
    if (options.debug || options.errors) {
        sprintf(message, "Field/member '%s' not found", rec);
        errorMessage(ERR_ST, message);
    }
}

int styyErrorRecovery(void) {
    if (options.debug && displayingErrorMessages()) {
        errorMessage(ERR_ST, "recovery");
    }
    return 0;
}

void setToNull(void *p) {
    void **pp;
    pp = (void **)p;
    *pp = NULL;
}

void deleteSymDef(void *p) {
    Symbol        *pp;

    pp = (Symbol *) p;
    log_debug("deleting %s %s", pp->name, pp->linkName);
    if (symbolTableDelete(s_javaStat->locals,pp)) return;
    if (symbolTableDelete(symbolTable,pp)==0) {
        assert(options.taskRegime);
        if (options.taskRegime != RegimeEditServer) {
            errorMessage(ERR_INTERNAL,"symbol on deletion not found");
        }
    }
}

void unpackPointers(Symbol *symbol) {
    for (int i=0; i<symbol->bits.npointers; i++) {
        appendComposedType(&symbol->u.typeModifier, TypePointer);
    }
    symbol->bits.npointers=0;
}

void addSymbol(Symbol *symbol, SymbolTable *table) {
    /*  a bug can produce, if you add a symbol into old table, and the same
        symbol exists in a newer one. Then it will be deleted from the newer
        one. All this story is about storing information in trail. It should
        containt, both table and pointer !!!!
    */
    log_debug("adding symbol %s: %s %s", symbol->name, typeNamesTable[symbol->bits.symbolType],
              storageEnumName[symbol->bits.storage]);
    assert(symbol->bits.npointers==0);
    AddSymbolNoTrail(symbol, table);
    addToTrail(deleteSymDef, symbol /* TODO? Should also include reference to table */, (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)));
}

void recFindPush(Symbol *str, S_recFindStr *rfs) {
    S_symStructSpec *ss;

    assert(str && (str->bits.symbolType==TypeStruct || str->bits.symbolType==TypeUnion));
    if (rfs->recsClassCounter==0) {
        // this is hack to avoid problem when overloading to zero
        rfs->recsClassCounter++;
    }
    ss = str->u.structSpec;
    rfs->nextRecord = ss->records;
    rfs->currClass = str;
    rfs->st[rfs->sti] = ss->super;
    assert(rfs->sti < MAX_INHERITANCE_DEEP);
    rfs->sti ++;
}

S_recFindStr *iniFind(Symbol *s, S_recFindStr *rfs) {
    assert(s);
    assert(s->bits.symbolType == TypeStruct || s->bits.symbolType == TypeUnion);
    assert(s->u.structSpec);
    assert(rfs);
    fillRecFindStr(rfs, s, NULL, NULL,s_recFindCl++);
    recFindPush(s, rfs);
    return rfs;
}

void setDirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        s_structRecordCompletionType = typeModifier;
        assert(s_structRecordCompletionType);
    }
}

void setIndirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        if (typeModifier->kind==TypePointer || typeModifier->kind==TypeArray) {
            s_structRecordCompletionType = typeModifier->next;
            assert(s_structRecordCompletionType);
        } else s_structRecordCompletionType = &s_errorModifier;
    }
}

bool javaOuterClassAccessible(Symbol *cl) {
    log_trace("testing class accessibility of %s",cl->linkName);
    if (cl->bits.access & AccessPublic) {
        log_trace("return true for public access");
        return true;
    }
    /* default access, check whether it is in current package */
    assert(s_javaStat);
    if (javaClassIsInCurrentPackage(cl)) {
        log_trace("return true for default protection in current package");
        return true;
    }
    log_trace("return false on default");
    return false;

}

static bool javaRecordVisible(Symbol *appcl, Symbol *funcl, unsigned accessFlags) {
    // there is special case to check! Private symbols are not inherited!
    if (accessFlags & AccessPrivate) {
        // check classes to string equality, just to be sure
        if (appcl!=funcl && strcmp(appcl->linkName, funcl->linkName)!=0)
            return false;
    }
    return true;
}

static bool accessibleByDefaultAccessibility(S_recFindStr *rfs, Symbol *funcl) {
    int             i;
    Symbol        *cc;
    SymbolList    *sups;
    if (rfs==NULL) {
        // nested class checking, just check without inheritance checking
        return javaClassIsInCurrentPackage(funcl);
    }
    // check accessibilities over inheritance hierarchy
    if (! javaClassIsInCurrentPackage(rfs->baseClass)) {
        return false;
    }
    cc = rfs->baseClass;
    for(i=0; i<rfs->sti-1; i++) {
        assert(cc);
        for(sups=cc->u.structSpec->super; sups!=NULL; sups=sups->next) {
            if (sups->next==rfs->st[i]) break;
        }
        if (sups!=NULL && sups->next == rfs->st[i]) {
            if (! javaClassIsInCurrentPackage(sups->d))
                return false;
        }
        cc = sups->d;
    }
    assert(cc==rfs->currClass);
    return true;
}

// BERK, there is a copy of this function in jslsemact.c (jslRecordAccessible)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// when modifying this, you will need to change it there too
// So, why don't we extract this common functionality?!?!?!?
bool javaRecordAccessible(S_recFindStr *rfs, Symbol *appcl, Symbol *funcl, Symbol *rec, unsigned recAccessFlags) {
    S_javaStat          *cs, *lcs;
    int                 len;
    if (funcl == NULL)
        return true;  /* argument or local variable */
    log_trace("testing accessibility %s . %s of x%x",funcl->linkName,rec->linkName, recAccessFlags);
    assert(s_javaStat);
    if (recAccessFlags & AccessPublic) {
        log_trace("return true for access public");
        return true;
    }
    if (recAccessFlags & AccessProtected) {
        // doesn't it refers to application class?
        if (accessibleByDefaultAccessibility(rfs, funcl)) {
            log_trace("return true for protected in current package");
            return true;
        }
        for (cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            if (cs->thisClass == funcl) {
                log_trace("return true for inside class");
                return true;
            }
            if (cctIsMember(&cs->thisClass->u.structSpec->casts, funcl, 1)) {
                log_trace("return true for inside subclass");
                return true;
            }
        }
        log_trace("return false on protected");
        return false;
    }
    if (recAccessFlags & AccessPrivate) {
        for(lcs=cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            lcs = cs;
        }
        if (lcs!=NULL && lcs->thisClass!=NULL) {
            //&fprintf(dumpOut,"comparing %s and %s\n", lcs->thisClass->linkName, funcl->linkName);
            len = strlen(lcs->thisClass->linkName);
            if (strncmp(lcs->thisClass->linkName, funcl->linkName, len)==0) {
                log_trace("return true for private inside the class");
                return true;
            }
        }
        log_trace("return false for private");
        return false;
    }
    /* default access */
    // it seems that here you should check rather if application class
    if (accessibleByDefaultAccessibility(rfs, funcl)) {
        log_trace("return true for default protection in current package");
        return true;
    }
    log_trace("return false for default");
    return false;
}

bool javaRecordVisibleAndAccessible(S_recFindStr *rfs, Symbol *applCl, Symbol *funCl, Symbol *r) {
    return javaRecordVisible(rfs->baseClass, rfs->currClass, r->bits.access)
        && javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, r->bits.access);
}

int javaGetMinimalAccessibility(S_recFindStr *rfs, Symbol *r) {
    int acc, i;
    for (i=MAX_REQUIRED_ACCESS; i>0; i--) {
        acc = javaRequiredAccessibilityTable[i];
        if (javaRecordVisible(rfs->baseClass, rfs->currClass, acc)
            && javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, acc)) {
            return i;
        }
    }
    return i;
}

#define FSRS_RETURN_WITH_SUCCESS(ss,res,r) {    \
        *res = r;                               \
        ss->nextRecord = r->next;               \
        return RETURN_OK;                       \
    }


#define FSRS_RETURN_WITH_FAIL(ss,res) {         \
        ss->nextRecord = NULL;                  \
        *res = &s_errorSymbol;                  \
        return RETURN_NOT_FOUND;                \
    }

int findStrRecordSym(S_recFindStr *ss,
                     char *recname,    /* can be NULL */
                     Symbol **res,
                     int javaClassif, /* classify to method/field*/
                     AccessibilityCheckYesNo accessibilityCheck,    /* java check accessibility */
                     VisibilityCheckYesNo visibilityCheck /* redundant, always equal to accCheck? */
) {
    Symbol            *s,*r,*cclass;
    SymbolList        *sss;
    int                 m;

    assert(accessibilityCheck == ACCESSIBILITY_CHECK_YES || accessibilityCheck == ACCESSIBILITY_CHECK_NO);
    assert(visibilityCheck == VISIBILITY_CHECK_YES || visibilityCheck == VISIBILITY_CHECK_NO);

    //&fprintf(dumpOut,":\nNEW SEARCH\n"); fflush(dumpOut);
    for(;;) {
        assert(ss);
        cclass = ss->currClass;
        if (cclass!=NULL&&cclass->u.structSpec->recSearchCounter==ss->recsClassCounter){
            // to avoid multiple pass through the same super-class ??
            //&fprintf(dumpOut,":%d==%d --> skipping class %s\n",cclass->u.structSpec->recSearchCounter,ss->recsClassCounter,cclass->linkName);
            goto nextClass;
        }
        //&if(cclass!=NULL)fprintf(dumpOut,":looking in class %s(%d)\n",cclass->linkName,ss->sti); fflush(dumpOut);
        for(r=ss->nextRecord; r!=NULL; r=r->next) {
            // special gcc extension of anonymous struct record
            if (r->name!=NULL && *r->name==0 && r->bits.symbolType==TypeDefault
                && r->u.typeModifier->kind==TypeAnonymousField
                && r->u.typeModifier->next!=NULL
                && (r->u.typeModifier->next->kind==TypeUnion || r->u.typeModifier->next->kind==TypeStruct)) {
                // put the anonymous union as 'super class'
                if (ss->aui+1 < MAX_ANONYMOUS_FIELDS) {
                    ss->au[ss->aui++] = r->u.typeModifier->next->u.t;
                }
            }
            //&fprintf(dumpOut,":checking %s\n",r->name); fflush(dumpOut);
            if (recname==NULL || strcmp(r->name,recname)==0) {
                if (! LANGUAGE(LANG_JAVA)) {
                    FSRS_RETURN_WITH_SUCCESS(ss, res, r);
                }
                //&fprintf(dumpOut,"acc O.K., checking classif %d\n",javaClassif);fflush(dumpOut);
                if (javaClassif!=CLASS_TO_ANY) {
                    assert(r->bits.symbolType == TypeDefault);
                    assert(r->u.typeModifier);
                    m = r->u.typeModifier->kind;
                    if (m==TypeFunction && javaClassif!=CLASS_TO_METHOD) goto nextRecord;
                    if (m!=TypeFunction && javaClassif==CLASS_TO_METHOD) goto nextRecord;
                }
                //&if(cclass!=NULL)fprintf(dumpOut,"name O.K., checking accesibility %xd %xd\n",cclass->bits.access,r->bits.access); fflush(dumpOut);
                // I have it, check visibility and accessibility
                assert(r);
                if (visibilityCheck == VISIBILITY_CHECK_YES) {
                    if (! javaRecordVisible(ss->baseClass, cclass, r->bits.access)) {
                        // WRONG? return, Doesn't it iverrides any other of this name
                        // Yes, definitely correct, in the first step determining
                        // class to search
                        FSRS_RETURN_WITH_FAIL(ss, res);
                    }
                }
                if (accessibilityCheck == ACCESSIBILITY_CHECK_YES) {
                    if (! javaRecordAccessible(ss, ss->baseClass,cclass,r,r->bits.access)){
                        if (visibilityCheck == VISIBILITY_CHECK_YES) {
                            FSRS_RETURN_WITH_FAIL(ss, res);
                        } else {
                            goto nextRecord;
                        }
                    }
                }
                FSRS_RETURN_WITH_SUCCESS(ss, res, r);
            }
        nextRecord:;
        }
    nextClass:
        if (ss->aui!=0) {
            // O.K. try first to pas to anonymous record
            s = ss->au[--ss->aui];
        } else {
            // mark the class as processed
            if (cclass!=NULL) {
                cclass->u.structSpec->recSearchCounter = ss->recsClassCounter;
            }

            while (ss->sti>0 && ss->st[ss->sti-1]==NULL) ss->sti--;
            if (ss->sti==0) {
                FSRS_RETURN_WITH_FAIL(ss, res);
            }
            sss = ss->st[ss->sti-1];
            s = sss->d;
            ss->st[ss->sti-1] = sss->next;
            assert(s && (s->bits.symbolType==TypeStruct || s->bits.symbolType==TypeUnion));
            //&fprintf(dumpOut,":pass to super class %s(%d)\n",s->linkName,ss->sti); fflush(dumpOut);
        }
        recFindPush(s, ss);
    }
}

int findStrRecord(Symbol *s,
                  char *recname,   /* can be NULL */
                  Symbol **res,
                  int javaClassif
) {
    S_recFindStr rfs;
    return findStrRecordSym(iniFind(s,&rfs), recname, res, javaClassif,
                           ACCESSIBILITY_CHECK_YES, VISIBILITY_CHECK_YES);
}

/* and push reference */
// this should be split into two copies, different for C and Java.
Reference *findStrRecordFromSymbol(Symbol *sym,
                                   Id *record,
                                   Symbol **res,
                                   int javaClassif,
                                   Id *super /* covering special case when invoked
                                                as SUPER.sym, berk */
) {
    S_recFindStr    rfs;
    Reference     *ref;
    Usage     usage;
    int rr, minacc;
    ref = NULL;
    // when in java, then always in qualified name, so access and visibility checks
    // are useless.
    rr = findStrRecordSym(iniFind(sym,&rfs),record->name,res,
                          javaClassif, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO);
    if (rr == RESULT_OK && rfs.currClass!=NULL &&
        ((*res)->bits.storage==StorageField
         || (*res)->bits.storage==StorageMethod
         || (*res)->bits.storage==StorageConstructor)){
        assert(rfs.currClass->u.structSpec && rfs.baseClass && rfs.baseClass->u.structSpec);
        if ((options.ooChecksBits & OOC_ALL_CHECKS)==0
            || javaRecordVisibleAndAccessible(&rfs, rfs.baseClass, rfs.currClass, *res)) {
            minacc = javaGetMinimalAccessibility(&rfs, *res);
            fillUsage(&usage, UsageUsed, minacc);
            ref = addNewCxReference(*res,&record->position, usage,
                                    rfs.currClass->u.structSpec->classFileIndex,
                                    rfs.baseClass->u.structSpec->classFileIndex);
            // this is adding reference to 'super', not to the field!
            // for pull-up/push-down
            if (super!=NULL) addThisCxReferences(s_javaStat->classFileIndex,&super->position);
        }
    } else if (rr == RESULT_OK) {
        ref = addCxReference(*res,&record->position,UsageUsed, noFileIndex, noFileIndex);
    } else {
        noSuchFieldError(record->name);
    }
    return ref;
}

Reference *findStructureFieldFromType(TypeModifier *structure,
                                        Id *field,
                                        Symbol **resultingSymbol,
                                        int javaClassifier) {
    Reference *reference = NULL;

    assert(structure);
    if (structure->kind != TypeStruct && structure->kind != TypeUnion) {
        *resultingSymbol = &s_errorSymbol;
        goto fini;
    }
    reference = findStrRecordFromSymbol(structure->u.t, field, resultingSymbol, javaClassifier, NULL);
 fini:
    return reference;
}

void labelReference(Id *id, UsageKind usage) {
    char ttt[TMP_STRING_SIZE];
    char *tt;
    assert(id);
    if (LANGUAGE(LANG_JAVA)) {
        assert(s_javaStat&&s_javaStat->thisClass&&s_javaStat->thisClass->u.structSpec);
        if (s_cp.function!=NULL) {
            sprintf(ttt,"%x-%s.%s",s_javaStat->thisClass->u.structSpec->classFileIndex,
                    s_cp.function->name, id->name);
        } else {
            sprintf(ttt,"%x-.%s", s_javaStat->thisClass->u.structSpec->classFileIndex,
                    id->name);
        }
    } else if (s_cp.function!=NULL) {
        tt = strmcpy(ttt, s_cp.function->name);
        *tt = '.';
        tt = strcpy(tt+1,id->name);
    } else {
        strcpy(ttt, id->name);
    }
    assert(strlen(ttt)<TMP_STRING_SIZE-1);
    addTrivialCxReference(ttt, TypeLabel,StorageDefault, &id->position, usage);
}

void setLocalVariableLinkName(struct symbol *p) {
    char ttt[TMP_STRING_SIZE];
    char nnn[TMP_STRING_SIZE];
    int len,tti;
    if (options.server_operation == OLO_EXTRACT) {
        // extract variable, must pass all needed informations in linkname
        sprintf(nnn, "%c%s%c", LINK_NAME_SEPARATOR, p->name, LINK_NAME_SEPARATOR);
        ttt[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
        // why it commented out ?
        //& if ((!LANGUAGE(LANG_JAVA))
        //&     && (p->u.typeModifier->kind == TypeUnion || p->u.typeModifier->kind == TypeStruct)) {
        //&     ttt[0] = LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
        //& }
        sprintf(ttt+1,"%s", storageNamesTable[p->bits.storage]);
        tti = strlen(ttt);
        len = TMP_STRING_SIZE - tti;
        typeSPrint(ttt+tti, &len, p->u.typeModifier, nnn, LINK_NAME_SEPARATOR, 0,1,SHORT_NAME, NULL);
        sprintf(ttt+tti+len,"%c%x-%x-%x-%x", LINK_NAME_SEPARATOR,
                p->pos.file,p->pos.line,p->pos.col, counters.localVar++);
    } else {
        if (p->bits.storage==StorageExtern && ! options.exactPositionResolve) {
            sprintf(ttt,"%s", p->name);
        } else {
            sprintf(ttt,"%x-%x-%x%c%s",p->pos.file,p->pos.line,p->pos.col,
                    LINK_NAME_SEPARATOR, p->name);
        }
    }
    len = strlen(ttt);
    p->linkName = StackMemoryAllocC(len+1, char);
    strcpy(p->linkName,ttt);
}

static void setStaticFunctionLinkName( Symbol *p, int usage ) {
    char        ttt[TMP_STRING_SIZE];
    int         len;
    char        *ss,*basefname;

    //& if (! symbolTableIsMember(symbolTable, p, &ii, &memb)) {
    // follwing unifies static symbols taken from the same header files.
    // Static symbols can be used only after being defined, so it is sufficient
    // to do this on definition usage?
    // With exactPositionResolve interpret them as distinct symbols for
    // each compilation unit.
    if (usage==UsageDefined && ! options.exactPositionResolve) {
        basefname=getFileItem(p->pos.file)->name;
    } else {
        basefname=inputFilename;
    }
    sprintf(ttt,"%s!%s", simpleFileName(basefname), p->name);
    len = strlen(ttt);
    assert(len < TMP_STRING_SIZE-2);
    ss = StackMemoryAllocC(len+1, char);
    strcpy(ss, ttt);
    p->linkName = ss;
    //& } else {
    //&     p->linkName=memb->linkName;
    //& }
}


Symbol *addNewSymbolDefinition(Symbol *p, unsigned theDefaultStorage, SymbolTable *tab,
                        UsageKind usage) {
    TypeModifier *tt;

    if (p == &s_errorSymbol || p->bits.symbolType==TypeError)
        return p;
    if (p->bits.symbolType == TypeError)
        return p;
    assert(p && p->bits.symbolType == TypeDefault && p->u.typeModifier);
    if (p->u.typeModifier->kind == TypeFunction && p->bits.storage == StorageDefault) {
        p->bits.storage = StorageExtern;
    }
    if (p->bits.storage == StorageDefault) {
        p->bits.storage = theDefaultStorage;
    }
    if (p->bits.symbolType==TypeDefault && p->bits.storage==StorageTypedef) {
        // typedef HACK !!!
        tt = StackMemoryAlloc(TypeModifier);
        *tt = *p->u.typeModifier;
        p->u.typeModifier = tt;
        tt->typedefSymbol = p;
    }
    if (nestingLevel() != 0) {
        // local scope symbol
        setLocalVariableLinkName(p);
    } else if (p->bits.symbolType==TypeDefault && p->bits.storage==StorageStatic) {
        setStaticFunctionLinkName(p, usage);
    }
    addSymbol(p, tab);
    addCxReference(p, &p->pos, usage,noFileIndex, noFileIndex);
    return p;
}

static void addInitializerRefs(Symbol *decl,
                               IdList *idl
                               ) {
    IdList *ll;
    Id* id;
    TypeModifier *tt;
    Reference *ref;
    Symbol *rec=NULL;
    for(ll=idl; ll!=NULL; ll=ll->next) {
        tt = decl->u.typeModifier;
        for (id = &ll->id; id!=NULL; id=id->next) {
            if (tt->kind == TypeArray) {
                tt = tt->next;
                continue;
            }
            if (tt->kind != TypeStruct && tt->kind != TypeUnion) return;
            ref = findStructureFieldFromType(tt, id, &rec, CLASS_TO_ANY);
            if (NULL == ref) return;
            assert(rec);
            tt = rec->u.typeModifier;
        }
    }
}

Symbol *addNewDeclaration(Symbol *btype, Symbol *decl, IdList *idl, unsigned storage, SymbolTable *tab) {
    UsageKind usageKind = UsageDefined;

    if (decl == &s_errorSymbol || btype == &s_errorSymbol || decl->bits.symbolType == TypeError ||
        btype->bits.symbolType == TypeError) {
        return decl;
    }
    assert(decl->bits.symbolType == TypeDefault);
    completeDeclarator(btype, decl);

    if (decl->u.typeModifier->kind == TypeFunction)
        usageKind = UsageDeclared;
    else if (decl->bits.storage == StorageExtern)
        usageKind = UsageDeclared;
    addNewSymbolDefinition(decl, storage, tab, usageKind);
    addInitializerRefs(decl, idl);
    return decl;
}

void addFunctionParameterToSymTable(Symbol *function, Symbol *p, int i, SymbolTable *tab) {
    Symbol *pp, *pa, *ppp;

    if (p->name != NULL && p->bits.symbolType!=TypeError) {
        assert(s_javaStat->locals!=NULL);
        pa = newSymbolAsCopyOf(p);

        // here checks a special case, double argument definition do not
        // redefine him, so refactorings will detect problem
        for(pp=function->u.typeModifier->u.f.args; pp!=NULL && pp!=p; pp=pp->next) {
            if (pp->name!=NULL && pp->bits.symbolType!=TypeError) {
                if (p!=pp && strcmp(pp->name, p->name)==0) break;
            }
        }
        if (pp!=NULL && pp!=p) {
            if (symbolTableIsMember(tab, pa, NULL, &ppp)) {
                addCxReference(ppp, &p->pos, UsageUsed, noFileIndex, noFileIndex);
            }
        } else {
            addNewSymbolDefinition(pa, StorageAuto, tab, UsageDefined);
        }
        if (options.server_operation == OLO_EXTRACT) {
            addCxReference(pa, &pa->pos, UsageLvalUsed,
                           noFileIndex, noFileIndex);
        }
    }
    if (options.server_operation == OLO_GOTO_PARAM_NAME
        && i == options.olcxGotoVal
        && positionsAreEqual(function->pos, s_cxRefPos)) {
        s_paramPosition = p->pos;
    }
}

static TypeModifier *createSimpleTypeModifier(Type type) {
    TypeModifier *p;

    /* This seems to look first in pre-created types... */
    assert(type>=0 && type<MAX_TYPE);
    if (s_preCreatedTypesTable[type] == NULL) {
        log_trace("creating simple type %d (='%s'), *not* found in pre-created types", type,
                  typeNamesTable[type]);
        p = newSimpleTypeModifier(type);
    } else {
        log_trace("creating simple type %d (='%s'), found in pre-created types", type,
                  typeNamesTable[type]);
        p = s_preCreatedTypesTable[type];
    }
    assert(p->kind == type);

    return p;
}

static TypeModifier *mergeBaseType(TypeModifier *t1,TypeModifier *t2){
    unsigned b,r;
    unsigned modif;
    assert(t1->kind<TYPE_MODIFIERS_END && t2->kind<TYPE_MODIFIERS_END);
    b=t1->kind; modif=t2->kind;// just to confuse compiler warning
    /* if both are types, error, return the new one only*/
    if (t1->kind <= MODIFIERS_START && t2->kind <= MODIFIERS_START)
        return t2;
    /* if not use tables*/
    if (t1->kind > MODIFIERS_START) {modif = t1->kind; b = t2->kind; }
    if (t2->kind > MODIFIERS_START) {modif = t2->kind; b = t1->kind; }
    switch (modif) {
    case TmodLong:
        r = typeLongChange[b];
        break;
    case TmodShort:
        r = typeShortChange[b];
        break;
    case TmodSigned:
        r = typeSignedChange[b];
        break;
    case TmodUnsigned:
        r = typeUnsignedChange[b];
        break;
    case TmodShortSigned:
        r = typeSignedChange[b];
        r = typeShortChange[r];
        break;
    case TmodShortUnsigned:
        r = typeUnsignedChange[b];
        r = typeShortChange[r];
        break;
    case TmodLongSigned:
        r = typeSignedChange[b];
        r = typeLongChange[r];
        break;
    case TmodLongUnsigned:
        r = typeUnsignedChange[b];
        r = typeLongChange[r];
        break;
    default: assert(0); r=0;
    }
    return createSimpleTypeModifier(r);
}

static TypeModifier * mergeBaseModTypes(TypeModifier *t1, TypeModifier *t2) {
    assert(t1 && t2);
    if (t1->kind == TypeDefault) return t2;
    if (t2->kind == TypeDefault) return t1;
    assert(t1->kind >=0 && t1->kind<MAX_TYPE);
    assert(t2->kind >=0 && t2->kind<MAX_TYPE);
    if (s_preCreatedTypesTable[t2->kind] == NULL) return t2;  /* not base type */
    if (s_preCreatedTypesTable[t1->kind] == NULL) return t1;  /* not base type */
    return mergeBaseType(t1, t2);
}

Symbol *typeSpecifier2(TypeModifier *t) {
    Symbol    *r;

    r = StackMemoryAlloc(Symbol);
    fillSymbolWithTypeModifier(r, NULL, NULL, noPosition, t);

    return r;
}

Symbol *typeSpecifier1(unsigned t) {
    Symbol        *r;
    r = typeSpecifier2(createSimpleTypeModifier(t));
    return r;
}

void declTypeSpecifier1(Symbol *d, Type type) {
    assert(d && d->u.typeModifier);
    d->u.typeModifier = mergeBaseModTypes(d->u.typeModifier,createSimpleTypeModifier(type));
}

void declTypeSpecifier2(Symbol *d, TypeModifier *t) {
    assert(d && d->u.typeModifier);
    d->u.typeModifier = mergeBaseModTypes(d->u.typeModifier, t);
}

void declTypeSpecifier21(TypeModifier *t, Symbol *d) {
    assert(d && d->u.typeModifier);
    d->u.typeModifier = mergeBaseModTypes(t, d->u.typeModifier);
}

TypeModifier *appendComposedType(TypeModifier **d, Type type) {
    TypeModifier *p;
    p = newTypeModifier(type, NULL, NULL);
    LIST_APPEND(TypeModifier, (*d), p);
    return p;
}

TypeModifier *prependComposedType(TypeModifier *d, Type type) {
    return newTypeModifier(type, NULL, d);
}

void completeDeclarator(Symbol *type, Symbol *declarator) {
    TypeModifier *tt,**dt;
    //static int counter=0;
    assert(type && declarator);
    if (type == &s_errorSymbol || declarator == &s_errorSymbol
        || type->bits.symbolType==TypeError || declarator->bits.symbolType==TypeError) return;
    declarator->bits.storage = type->bits.storage;
    assert(type->bits.symbolType==TypeDefault);
    dt = &(declarator->u.typeModifier); tt = type->u.typeModifier;
    if (declarator->bits.npointers) {
        if (declarator->bits.npointers>=1 && (tt->kind==TypeStruct||tt->kind==TypeUnion)
            && tt->typedefSymbol==NULL) {
            //fprintf(dumpOut,"saving 1 str pointer:%d\n",counter++);fflush(dumpOut);
            declarator->bits.npointers--;
            //if(d->b.npointers) {fprintf(dumpOut,"possible 2\n");fflush(dumpOut);}
            assert(tt->u.t && tt->u.t->bits.symbolType==tt->kind && tt->u.t->u.structSpec);
            tt = & tt->u.t->u.structSpec->sptrtype;
        } else if (declarator->bits.npointers>=2 && s_preCrPtr2TypesTab[tt->kind]!=NULL
                   && tt->typedefSymbol==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 2 pointer\n");fflush(dumpOut);
            declarator->bits.npointers-=2;
            tt = s_preCrPtr2TypesTab[tt->kind];
        } else if (declarator->bits.npointers>=1 && s_preCrPtr1TypesTab[tt->kind]!=NULL
                   && tt->typedefSymbol==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 1 pointer\n");fflush(dumpOut);
            declarator->bits.npointers--;
            tt = s_preCrPtr1TypesTab[tt->kind];
        }
    }
    unpackPointers(declarator);
    LIST_APPEND(TypeModifier, *dt, tt);
}

Symbol *createSimpleDefinition(Storage storage, Type type, Id *id) {
    TypeModifier *typeModifier;
    Symbol *r;

    typeModifier = newTypeModifier(type, NULL, NULL);
    if (id!=NULL) {
        r = newSymbolAsType(id->name, id->name, id->position, typeModifier);
    } else {
        r = newSymbolAsType(NULL, NULL, noPosition, typeModifier);
    }
    fillSymbolBits(&r->bits, AccessDefault, TypeDefault, storage);

    return r;
}

SymbolList *createDefinitionList(Symbol *symbol) {
    SymbolList *p;

    assert(symbol);
    p = StackMemoryAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(p, symbol, NULL); with compound literal */
    *p = (SymbolList){.d = symbol, .next = NULL};

    return p;
}

int mergeArguments(Symbol *id, Symbol *ty) {
    Symbol *p;
    int res;
    res = RESULT_OK;
    /* if a type of non-exist. argument is declared, it is probably */
    /* only a missing ';', so syntax error should be raised */
    for(;ty!=NULL; ty=ty->next) {
        if (ty->name != NULL) {
            for(p=id; p!=NULL; p=p->next) {
                if (p->name!=NULL && strcmp(p->name,ty->name)==0) break;
            }
            if (p==NULL) res = RESULT_ERR;
            else {
                if (p->u.typeModifier == NULL) p->u.typeModifier = ty->u.typeModifier;
            }
        }
    }
    return res;
}

static TypeModifier *createSimpleEnumType(Symbol *enumDefinition) {
    return newEnumTypeModifier(enumDefinition);
}

void initSymStructSpec(S_symStructSpec *symStruct, Symbol *records) {
    memset((void*)symStruct, 0, sizeof(*symStruct));
    symStruct->records = records;
    symStruct->classFileIndex = -1;  /* Should be s_noFile? */
}

TypeModifier *simpleStrUnionSpecifier(Id *typeName,
                                      Id *id,
                                      UsageKind usage
) {
    Symbol p,*pp;
    int type;

    log_trace("new struct %s", id->name);
    assert(typeName && typeName->symbol && typeName->symbol->bits.symbolType == TypeKeyword);
    assert(     typeName->symbol->u.keyword == STRUCT
                ||  typeName->symbol->u.keyword == CLASS
                ||  typeName->symbol->u.keyword == UNION
                );
    if (typeName->symbol->u.keyword != UNION) type = TypeStruct;
    else type = TypeUnion;

    fillSymbol(&p, id->name, id->name, id->position);
    fillSymbolBits(&p.bits, AccessDefault, type, StorageNone);

    if (!symbolTableIsMember(symbolTable, &p, NULL, &pp)
        || (isMemoryFromPreviousBlock(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        //{static int c=0;fprintf(dumpOut,"str#%d\n",c++);}
        pp = StackMemoryAlloc(Symbol);
        *pp = p;
        pp->u.structSpec = StackMemoryAlloc(S_symStructSpec);

        initSymStructSpec(pp->u.structSpec, /*.records=*/NULL);
        TypeModifier *stype = &pp->u.structSpec->stype;
        /* Assumed to be Struct/Union/Enum? */
        initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/pp,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
        TypeModifier *sptrtype = &pp->u.structSpec->sptrtype;
        initTypeModifierAsPointer(sptrtype, &pp->u.structSpec->stype);

        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbol(pp, symbolTable);
    }
    addCxReference(pp, &id->position, usage,noFileIndex, noFileIndex);
    return &pp->u.structSpec->stype;
}

void setGlobalFileDepNames(char *iname, Symbol *symbol, int memory) {
    char *mname, *fname;
    char tmp[MACRO_NAME_SIZE];
    Symbol *memb;
    int isMember, order, len, len2;

    if (iname == NULL)
        iname="";
    assert(symbol);
    if (options.exactPositionResolve) {
        FileItem *fileItem = getFileItem(symbol->pos.file);
        fname = simpleFileName(fileItem->name);
        sprintf(tmp, "%x-%s-%x-%x%c", hashFun(fileItem->name), fname, symbol->pos.line, symbol->pos.col,
                LINK_NAME_SEPARATOR);
    } else if (iname[0]==0) {
        // anonymous enum/structure/union ...
        int fileIndex = symbol->pos.file;
        symbol->name=iname;
        symbol->linkName=iname;
        order = 0;
        isMember = symbolTableIsMember(symbolTable, symbol, NULL, &memb);
        while (isMember) {
            if (memb->pos.file==fileIndex)
                order++;
            isMember = symbolTableNextMember(symbol, &memb);
        }
        fname = simpleFileName(getFileItem(fileIndex)->name);
        sprintf(tmp, "%s%c%d%c", fname, FILE_PATH_SEPARATOR, order, LINK_NAME_SEPARATOR);
        /*&     // macros will be identified by name only?
          } else if (symbol->bits.symbolType == TypeMacro) {
          sprintf(tmp, "%x%c", symbol->pos.file, LINK_NAME_SEPARATOR);
          &*/
    } else {
        tmp[0] = 0;
    }
    len = strlen(tmp);
    len2 = len + strlen(iname);
    assert(len < MACRO_NAME_SIZE-2);
    if (memory == MEMORY_XX) {
        mname = StackMemoryAllocC(len2+1, char);
    } else {
        PPM_ALLOCC(mname, len2+1, char);
    }
    strcpy(mname, tmp);
    strcpy(mname+len,iname);
    symbol->name = mname + len;
    symbol->linkName = mname;
}

TypeModifier *createNewAnonymousStructOrUnion(Id *typeName) {
    Symbol *pp;
    int type;

    assert(typeName);
    assert(typeName->symbol);
    assert(typeName->symbol->bits.symbolType == TypeKeyword);
    assert(typeName->symbol->u.keyword == STRUCT
           ||  typeName->symbol->u.keyword == CLASS
           ||  typeName->symbol->u.keyword == UNION
           );
    if (typeName->symbol->u.keyword == STRUCT) type = TypeStruct;
    else type = TypeUnion;

    pp = newSymbol("", NULL, typeName->position);
    fillSymbolBits(&pp->bits, AccessDefault, type, StorageNone);

    setGlobalFileDepNames("", pp, MEMORY_XX);

    pp->u.structSpec = StackMemoryAlloc(S_symStructSpec);

    /* This is a recurring pattern, create a struct and the pointer type to it*/
    initSymStructSpec(pp->u.structSpec, /*.records=*/NULL);
    TypeModifier *stype = &pp->u.structSpec->stype;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/pp,
                                        /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *sptrtype = &pp->u.structSpec->sptrtype;
    initTypeModifierAsPointer(sptrtype, &pp->u.structSpec->stype);

    addSymbol(pp, symbolTable);

    return &pp->u.structSpec->stype;
}

void specializeStrUnionDef(Symbol *sd, Symbol *rec) {
    Symbol *dd;
    assert(sd->bits.symbolType == TypeStruct || sd->bits.symbolType == TypeUnion);
    assert(sd->u.structSpec);
    if (sd->u.structSpec->records!=NULL) return;
    sd->u.structSpec->records = rec;
    addToTrail(setToNull, & (sd->u.structSpec->records), (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)));
    for(dd=rec; dd!=NULL; dd=dd->next) {
        if (dd->name!=NULL) {
            dd->linkName = string3ConcatInStackMem(sd->linkName,".",dd->name);
            dd->bits.isRecord = 1;
            addCxReference(dd,&dd->pos,UsageDefined,noFileIndex, noFileIndex);
        }
    }
}

TypeModifier *simpleEnumSpecifier(Id *id, UsageKind usage) {
    Symbol p,*pp;

    fillSymbol(&p, id->name, id->name, id->position);
    fillSymbolBits(&p.bits, AccessDefault, TypeEnum, StorageNone);

    if (! symbolTableIsMember(symbolTable, &p, NULL, &pp)
        || (isMemoryFromPreviousBlock(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        pp = StackMemoryAlloc(Symbol);
        *pp = p;
        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbol(pp, symbolTable);
    }
    addCxReference(pp, &id->position, usage,noFileIndex, noFileIndex);
    return createSimpleEnumType(pp);
}

TypeModifier *createNewAnonymousEnum(SymbolList *enums) {
    Symbol *symbol;

    symbol = newSymbolAsEnum("", "", noPosition, enums);
    fillSymbolBits(&symbol->bits, AccessDefault, TypeEnum, StorageNone);

    setGlobalFileDepNames("", symbol, MEMORY_XX);
    symbol->u.enums = enums;
    return createSimpleEnumType(symbol);
}

void appendPositionToList(PositionList **list, Position *pos) {
    PositionList *ppl;
    ppl = StackMemoryAlloc(PositionList);
    fillPositionList(ppl, *pos, NULL);
    LIST_APPEND(PositionList, (*list), ppl);
}

void setParamPositionForFunctionWithoutParams(Position *lpar) {
    s_paramBeginPosition = *lpar;
    s_paramEndPosition = *lpar;
}

void setParamPositionForParameter0(Position *lpar) {
    s_paramBeginPosition = *lpar;
    s_paramEndPosition = *lpar;
}

void setParamPositionForParameterBeyondRange(Position *rpar) {
    s_paramBeginPosition = *rpar;
    s_paramEndPosition = *rpar;
}

static void handleParameterPositions(Position *lpar, PositionList *commas,
                                     Position *rpar, int hasParam) {
    int i, argn;
    Position *p1, *p2;
    PositionList *pp;
    if (! hasParam) {
        setParamPositionForFunctionWithoutParams(lpar);
        return;
    }
    argn = options.olcxGotoVal;
    if (argn == 0) {
        setParamPositionForParameter0(lpar);
    } else {
        pp = commas;
        p1 = lpar;
        i = 1;
        if (pp != NULL) p2 = &pp->p;
        else p2 = rpar;
        for(i++; pp!=NULL && i<=argn; pp=pp->next,i++) {
            p1 = &pp->p;
            if (pp->next != NULL) p2 = &pp->next->p;
            else p2 = rpar;
        }
        if (pp==NULL && i<=argn) {
            setParamPositionForParameterBeyondRange(rpar);
        } else {
            s_paramBeginPosition = *p1;
            s_paramEndPosition = *p2;
        }
    }
}

Symbol *createEmptyField(void) {
    TypeModifier *p;

    p = newSimpleTypeModifier(TypeAnonymousField);
    return newSymbolAsType("", "", noPosition, p);
}

void handleDeclaratorParamPositions(Symbol *decl, Position *lpar,
                                    PositionList *commas, Position *rpar,
                                    int hasParam
                                    ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (positionsAreNotEqual(decl->pos, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void handleInvocationParamPositions(Reference *ref, Position *lpar,
                                    PositionList *commas, Position *rpar,
                                    int hasParam
                                    ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (ref==NULL || positionsAreNotEqual(ref->position, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void javaHandleDeclaratorParamPositions(Position *sym, Position *lpar,
                                        PositionList *commas, Position *rpar
                                        ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (positionsAreNotEqual(*sym, s_cxRefPos)) return;
    if (commas==NULL) {
        handleParameterPositions(lpar, NULL, rpar, 0);
    } else {
        handleParameterPositions(lpar, commas->next, rpar, 1);
    }
}
