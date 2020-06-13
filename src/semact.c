#include "semact.h"

#include "commons.h"
#include "globals.h"
#include "parsers.h"
#include "misc.h"
#include "classcaster.h"
#include "yylex.h"
#include "cxref.h"
#include "jsemact.h"
#include "jslsemact.h"          /*& s_jsl &*/
#include "symbol.h"
#include "list.h"

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
    // no error messages for file preloaded for symbols
    if (LANGUAGE(LANG_JAVA) && s_jsl!=NULL)
        return false;
    if (options.debug || options.show_errors)
        return true;
    return false;
}

int styyerror(char *s) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (strcmp(s, "syntax error") != 0) {
        sprintf(tmpBuff,"YACC error: %s", s);
        errorMessage(ERR_INTERNAL, tmpBuff);
    } else if (displayingErrorMessages()) {
        sprintf(tmpBuff, "Syntax error on: %s", yytext);
        errorMessage(ERR_ST, tmpBuff);
    }
    return(0);
}

void noSuchFieldError(char *rec) {
    char message[TMP_BUFF_SIZE];
    if (options.debug || options.show_errors) {
        sprintf(message, "Field/member '%s' not found", rec);
        errorMessage(ERR_ST, message);
    }
}

int styyErrorRecovery(void) {
    if (options.debug && displayingErrorMessages()) {
        errorMessage(ERR_ST, "recovery");
    }
    return(0);
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
    if (symbolTableDelete(s_symbolTable,pp)==0) {
        assert(options.taskRegime);
        if (options.taskRegime != RegimeEditServer) {
            errorMessage(ERR_INTERNAL,"symbol on deletion not found");
        }
    }
}

void unpackPointers(Symbol *pp) {
    unsigned i;
    for (i=0; i<pp->bits.npointers; i++) {
        appendComposedType(&pp->u.type, TypePointer);
    }
    pp->bits.npointers=0;
}

void addSymbol(Symbol *pp, S_symbolTable *tab) {
    /*  a bug can produce, if you add a symbol into old table, and the same
        symbol exists in a newer one. Then it will be deleted from the newer
        one. All this story is about storing information in trail. It should
        containt, both table and pointer !!!!
    */
    log_debug("adding symbol %s: %s %s",pp->name, typeEnumName[pp->bits.symType], storageEnumName[pp->bits.storage]);
    assert(pp->bits.npointers==0);
    AddSymbolNoTrail(pp,tab);
    addToTrail(deleteSymDef, pp  /* AND ALSO!!! , tab */ );
    //if (WORK_NEST_LEVEL0()) {static int c=0;fprintf(dumpOut,"addsym0#%d\n",c++);}
}

void recFindPush(Symbol *str, S_recFindStr *rfs) {
    S_symStructSpec *ss;

    assert(str && (str->bits.symType==TypeStruct || str->bits.symType==TypeUnion));
    if (rfs->recsClassCounter==0) {
        // this is hack to avoid problem when overloading to zero
        rfs->recsClassCounter++;
    }
    ss = str->u.s;
    rfs->nextRecord = ss->records;
    rfs->currClass = str;
    //& if (ss->super != NULL) { // this optimization makes completion info wrong
    rfs->st[rfs->sti] = ss->super;
    assert(rfs->sti < MAX_INHERITANCE_DEEP);
    rfs->sti ++;
    //& }
}

S_recFindStr * iniFind(Symbol *s, S_recFindStr *rfs) {
    assert(s);
    assert(s->bits.symType == TypeStruct || s->bits.symType == TypeUnion);
    assert(s->u.s);
    assert(rfs);
    fillRecFindStr(rfs, s, NULL, NULL,s_recFindCl++);
    recFindPush(s, rfs);
    return(rfs);
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

static int javaRecordVisible(Symbol *appcl, Symbol *funcl, unsigned accessFlags) {
    // there is special case to check! Private symbols are not inherited!
    if (accessFlags & AccessPrivate) {
        // check classes to string equality, just to be sure
        if (appcl!=funcl && strcmp(appcl->linkName, funcl->linkName)!=0) return(0);
    }
    return(1);
}

static int accessibleByDefaultAccessibility(S_recFindStr *rfs, Symbol *funcl) {
    int             i;
    Symbol        *cc;
    SymbolList    *sups;
    if (rfs==NULL) {
        // nested class checking, just check without inheritance checking
        return(javaClassIsInCurrentPackage(funcl));
    }
    // check accessibilities over inheritance hierarchy
    if (! javaClassIsInCurrentPackage(rfs->baseClass)) {
        return(0);
    }
    cc = rfs->baseClass;
    for(i=0; i<rfs->sti-1; i++) {
        assert(cc);
        for(sups=cc->u.s->super; sups!=NULL; sups=sups->next) {
            if (sups->next==rfs->st[i]) break;
        }
        if (sups!=NULL && sups->next == rfs->st[i]) {
            if (! javaClassIsInCurrentPackage(sups->d)) return(0);
        }
        cc = sups->d;
    }
    assert(cc==rfs->currClass);
    return(1);
}

// BERK, there is a copy of this function in jslsemact.c (jslRecordAccessible)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// when modifying this, you will need to change it there too
// So, why don't we extract this common functionality?!?!?!?
int javaRecordAccessible(S_recFindStr *rfs, Symbol *appcl, Symbol *funcl, Symbol *rec, unsigned recAccessFlags) {
    S_javaStat          *cs, *lcs;
    int                 len;
    if (funcl == NULL) return(1);  /* argument or local variable */
    log_trace("testing accessibility %s . %s of x%x",funcl->linkName,rec->linkName, recAccessFlags);
    assert(s_javaStat);
    if (recAccessFlags & AccessPublic) {
        log_trace("ret 1 access public");
        return 1;
    }
    if (recAccessFlags & AccessProtected) {
        // doesn't it refers to application class?
        if (accessibleByDefaultAccessibility(rfs, funcl)) {
            log_trace("ret 1 protected in current package");
            return 1;
        }
        for (cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            if (cs->thisClass == funcl) {
                log_trace("ret 1 as it is inside class");
                return 1;
            }
            if (cctIsMember(&cs->thisClass->u.s->casts, funcl, 1)) {
                log_trace("ret 1 as it is inside subclass");
                return 1;
            }
        }
        log_trace("ret 0 on protected");
        return 0;
    }
    if (recAccessFlags & AccessPrivate) {
        for(lcs=cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            lcs = cs;
        }
        if (lcs!=NULL && lcs->thisClass!=NULL) {
            //&fprintf(dumpOut,"comparing %s and %s\n", lcs->thisClass->linkName, funcl->linkName);
            len = strlen(lcs->thisClass->linkName);
            if (strncmp(lcs->thisClass->linkName, funcl->linkName, len)==0) {
                log_trace("ret 1 private inside the class");
                return 1;
            }
        }
        log_trace("ret 0 on private");
        return 0;
    }
    /* default access */
    // it seems that here you should check rather if application class
    if (accessibleByDefaultAccessibility(rfs, funcl)) {
        log_trace("ret 1 default protection in current package");
        return 1;
    }
    log_trace("ret 0 on default");
    return 0;
}

int javaRecordVisibleAndAccessible(S_recFindStr *rfs, Symbol *applCl, Symbol *funCl, Symbol *r) {
    return(
           javaRecordVisible(rfs->baseClass, rfs->currClass, r->bits.access)
           &&
           javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, r->bits.access)
           );
}

int javaGetMinimalAccessibility(S_recFindStr *rfs, Symbol *r) {
    int acc, i;
    for(i=MAX_REQUIRED_ACCESS; i>0; i--) {
        acc = s_javaRequiredeAccessibilitiesTable[i];
        if (javaRecordVisible(rfs->baseClass, rfs->currClass, acc)
            && javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, acc)) {
            return(i);
        }
    }
    return(i);
}

#define FSRS_RETURN_WITH_SUCCESS(ss,res,r) {    \
        *res = r;                               \
        ss->nextRecord = r->next;               \
        return(RETURN_OK);                      \
    }


#define FSRS_RETURN_WITH_FAIL(ss,res) {         \
        ss->nextRecord = NULL;                  \
        *res = &s_errorSymbol;                  \
        return(RETURN_NOT_FOUND);               \
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
        if (cclass!=NULL&&cclass->u.s->recSearchCounter==ss->recsClassCounter){
            // to avoid multiple pass through the same super-class ??
            //&fprintf(dumpOut,":%d==%d --> skipping class %s\n",cclass->u.s->recSearchCounter,ss->recsClassCounter,cclass->linkName);
            goto nextClass;
        }
        //&if(cclass!=NULL)fprintf(dumpOut,":looking in class %s(%d)\n",cclass->linkName,ss->sti); fflush(dumpOut);
        for(r=ss->nextRecord; r!=NULL; r=r->next) {
            // special gcc extension of anonymous struct record
            if (r->name!=NULL && *r->name==0 && r->bits.symType==TypeDefault
                && r->u.type->kind==TypeAnonymousField
                && r->u.type->next!=NULL
                && (r->u.type->next->kind==TypeUnion || r->u.type->next->kind==TypeStruct)) {
                // put the anonymous union as 'super class'
                if (ss->aui+1 < MAX_ANONYMOUS_FIELDS) {
                    ss->au[ss->aui++] = r->u.type->next->u.t;
                }
            }
            //&fprintf(dumpOut,":checking %s\n",r->name); fflush(dumpOut);
            if (recname==NULL || strcmp(r->name,recname)==0) {
                if (! LANGUAGE(LANG_JAVA)) {
                    FSRS_RETURN_WITH_SUCCESS(ss, res, r);
                }
                //&fprintf(dumpOut,"acc O.K., checking classif %d\n",javaClassif);fflush(dumpOut);
                if (javaClassif!=CLASS_TO_ANY) {
                    assert(r->bits.symType == TypeDefault);
                    assert(r->u.type);
                    m = r->u.type->kind;
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
                cclass->u.s->recSearchCounter = ss->recsClassCounter;
            }

            while (ss->sti>0 && ss->st[ss->sti-1]==NULL) ss->sti--;
            if (ss->sti==0) {
                FSRS_RETURN_WITH_FAIL(ss, res);
            }
            sss = ss->st[ss->sti-1];
            s = sss->d;
            ss->st[ss->sti-1] = sss->next;
            assert(s && (s->bits.symType==TypeStruct || s->bits.symType==TypeUnion));
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
    return(findStrRecordSym(iniFind(s,&rfs), recname, res, javaClassif,
                            ACCESSIBILITY_CHECK_YES, VISIBILITY_CHECK_YES));
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
    UsageBits     ub;
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
        assert(rfs.currClass->u.s && rfs.baseClass && rfs.baseClass->u.s);
        if ((options.ooChecksBits & OOC_ALL_CHECKS)==0
            || javaRecordVisibleAndAccessible(&rfs, rfs.baseClass, rfs.currClass, *res)) {
            minacc = javaGetMinimalAccessibility(&rfs, *res);
            fillUsageBits(&ub, UsageUsed, minacc);
            ref = addCxReferenceNew(*res,&record->p, &ub,
                                    rfs.currClass->u.s->classFile,
                                    rfs.baseClass->u.s->classFile);
            // this is adding reference to 'super', not to the field!
            // for pull-up/push-down
            if (super!=NULL) addThisCxReferences(s_javaStat->classFileIndex,&super->p);
        }
    } else if (rr == RESULT_OK) {
        ref = addCxReference(*res,&record->p,UsageUsed, s_noneFileIndex, s_noneFileIndex);
    } else {
        noSuchFieldError(record->name);
    }
    return(ref);
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
    return(reference);
}

void labelReference(Id *id, int usage) {
    char ttt[TMP_STRING_SIZE];
    char *tt;
    assert(id);
    if (LANGUAGE(LANG_JAVA)) {
        assert(s_javaStat&&s_javaStat->thisClass&&s_javaStat->thisClass->u.s);
        if (s_cp.function!=NULL) {
            sprintf(ttt,"%x-%s.%s",s_javaStat->thisClass->u.s->classFile,
                    s_cp.function->name, id->name);
        } else {
            sprintf(ttt,"%x-.%s", s_javaStat->thisClass->u.s->classFile,
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
    addTrivialCxReference(ttt, TypeLabel,StorageDefault, &id->p, usage);
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
        //&     && (p->u.type->kind == TypeUnion || p->u.type->kind == TypeStruct)) {
        //&     ttt[0] = LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
        //& }
        sprintf(ttt+1,"%s", s_extractStorageName[p->bits.storage]);
        tti = strlen(ttt);
        len = TMP_STRING_SIZE - tti;
        typeSPrint(ttt+tti, &len, p->u.type, nnn, LINK_NAME_SEPARATOR, 0,1,SHORT_NAME, NULL);
        sprintf(ttt+tti+len,"%c%x-%x-%x-%x", LINK_NAME_SEPARATOR,
                p->pos.file,p->pos.line,p->pos.col, s_count.localVar++);
    } else {
        if (p->bits.storage==StorageExtern && ! options.exactPositionResolve) {
            sprintf(ttt,"%s", p->name);
        } else {
            // it is now better to have name allways accessible
            //&         if (options.taskRegime == RegimeHtmlGenerate) {
            // html symbol, must pass the name for cxreference list item
            sprintf(ttt,"%x-%x-%x%c%s",p->pos.file,p->pos.line,p->pos.col,
                    LINK_NAME_SEPARATOR, p->name);
            /*&
              } else {
              // no special information need to pass
              sprintf(ttt,"%x-%x-%x", p->pos.file,p->pos.line,p->pos.col);
              }
              &*/
        }
    }
    len = strlen(ttt);
    XX_ALLOCC(p->linkName, len+1, char);
    strcpy(p->linkName,ttt);
}

static void setStaticFunctionLinkName( Symbol *p, int usage ) {
    char        ttt[TMP_STRING_SIZE];
    int         len;
    char        *ss,*basefname;

    //& if (! symbolTableIsMember(s_symbolTable, p, &ii, &memb)) {
    // follwing unifies static symbols taken from the same header files.
    // Static symbols can be used only after being defined, so it is sufficient
    // to do this on definition usage?
    // With exactPositionResolve interpret them as distinct symbols for
    // each compilation unit.
    if (usage==UsageDefined && ! options.exactPositionResolve) {
        basefname=fileTable.tab[p->pos.file]->name;
    } else {
        basefname=s_input_file_name;
    }
    sprintf(ttt,"%s!%s", simpleFileName(basefname), p->name);
    len = strlen(ttt);
    assert(len < TMP_STRING_SIZE-2);
    XX_ALLOCC(ss, len+1, char);
    strcpy(ss, ttt);
    p->linkName = ss;
    //& } else {
    //&     p->linkName=memb->linkName;
    //& }
}

#define MEM_FROM_PREVIOUS_BLOCK(ppp) (                                          \
                                      s_topBlock->previousTopBlock != NULL &&   \
                                      ((char*)ppp) > memory &&					\
                                      ((char*)ppp) < memory+s_topBlock->previousTopBlock->firstFreeIndex \
                                      )

Symbol *addNewSymbolDef(Symbol *p, unsigned theDefaultStorage, S_symbolTable *tab,
                          int usage) {
    TypeModifier *tt;
    Symbol *pp;
    int ii;
    if (p == &s_errorSymbol || p->bits.symType==TypeError) return(p);
    if (p->bits.symType == TypeError) return(p);
    assert(p && p->bits.symType == TypeDefault && p->u.type);
    if (p->u.type->kind == TypeFunction && p->bits.storage == StorageDefault) {
        p->bits.storage = StorageExtern;
    }
    if (p->bits.storage == StorageDefault) {
        p->bits.storage = theDefaultStorage;
    }
    if (p->bits.symType==TypeDefault && p->bits.storage==StorageTypedef) {
        // typedef HACK !!!
        XX_ALLOC(tt, TypeModifier);
        *tt = *p->u.type;
        p->u.type = tt;
        tt->typedefSymbol = p;
    }
    if ((! WORK_NEST_LEVEL0() && LANGUAGE(LANG_C))
        || (! WORK_NEST_LEVEL1() && LANGUAGE(LANG_YACC))) {
        // local scope symbol
        if (! symbolTableIsMember(s_symbolTable,p,&ii,&pp)
            || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
            pp = p;
            setLocalVariableLinkName(pp);
            addSymbol(pp, tab);
        }
    } else if (p->bits.symType==TypeDefault && p->bits.storage==StorageStatic) {
        if (! symbolTableIsMember(s_symbolTable,p,&ii,&pp)) {
            pp = p;
            setStaticFunctionLinkName(pp, usage);
            addSymbol(pp, tab);
        }
    } else {
        if (! symbolTableIsMember(s_symbolTable,p,&ii,&pp)) {
            pp = p;
            if (options.exactPositionResolve) {
                setGlobalFileDepNames(pp->name, pp, MEMORY_XX);
            }
            addSymbol(pp, tab);
        }
    }
    addCxReference(pp, &p->pos, usage,s_noneFileIndex, s_noneFileIndex);
    return(pp);
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
        tt = decl->u.type;
        for (id = &ll->id; id!=NULL; id=id->next) {
            if (tt->kind == TypeArray) {
                tt = tt->next;
                continue;
            }
            if (tt->kind != TypeStruct && tt->kind != TypeUnion) return;
            ref = findStructureFieldFromType(tt, id, &rec, CLASS_TO_ANY);
            if (NULL == ref) return;
            assert(rec);
            tt = rec->u.type;
        }
    }
}

Symbol *addNewDeclaration(
                            Symbol *btype,
                            Symbol *decl,
                            IdList *idl,
                            unsigned storage,
                            S_symbolTable *tab
                            ) {
    int usage;
    if (decl == &s_errorSymbol || btype == &s_errorSymbol
        || decl->bits.symType==TypeError || btype->bits.symType==TypeError) {
        return(decl);
    }
    assert(decl->bits.symType == TypeDefault);
    completeDeclarator(btype, decl);
    usage = UsageDefined;
    if (decl->u.type->kind == TypeFunction) usage = UsageDeclared;
    else if (decl->bits.storage == StorageExtern) usage = UsageDeclared;
    addNewSymbolDef(decl, storage, tab, usage);
    addInitializerRefs(decl, idl);
    return(decl);
}

void addFunctionParameterToSymTable(Symbol *function, Symbol *p, int i, S_symbolTable *tab) {
    Symbol    *pp, *pa, *ppp;
    int         ii;
    if (p->name != NULL && p->bits.symType!=TypeError) {
        assert(s_javaStat->locals!=NULL);
        XX_ALLOC(pa, Symbol);
        *pa = *p;
        // here checks a special case, double argument definition do not
        // redefine him, so refactorings will detect problem
        for(pp=function->u.type->u.f.args; pp!=NULL && pp!=p; pp=pp->next) {
            if (pp->name!=NULL && pp->bits.symType!=TypeError) {
                if (p!=pp && strcmp(pp->name, p->name)==0) break;
            }
        }
        if (pp!=NULL && pp!=p) {
            if (symbolTableIsMember(tab, pa, &ii, &ppp)) {
                addCxReference(ppp, &p->pos, UsageUsed, s_noneFileIndex, s_noneFileIndex);
            }
        } else {
            addNewSymbolDef(pa, StorageAuto, tab, UsageDefined);
        }
        if (options.server_operation == OLO_EXTRACT) {
            addCxReference(pa, &pa->pos, UsageLvalUsed,
                           s_noneFileIndex, s_noneFileIndex);
        }
    }
    if (options.server_operation == OLO_GOTO_PARAM_NAME
        && i == options.olcxGotoVal
        && POSITION_EQ(function->pos, s_cxRefPos)) {
        s_paramPosition = p->pos;
    }
}

static TypeModifier *createSimpleTypeModifier(Type type) {
    TypeModifier *p;

    /* This seems to look first in pre-created types... */
    assert(type>=0 && type<MAX_TYPE);
    if (s_preCreatedTypesTable[type] == NULL) {
        log_trace("creating simple type %d (='%s'), *not* found in pre-created types", type,
                  typeEnumName[type]);
        p = newSimpleTypeModifier(type);
    } else {
        log_trace("creating simple type %d (='%s'), found in pre-created types", type,
                  typeEnumName[type]);
        p = s_preCreatedTypesTable[type];
    }
    assert(p->kind == type);

    return p;
}

static TypeModifier *mergeBaseType(TypeModifier *t1,TypeModifier *t2){
    unsigned b,r;
    unsigned modif;
    assert(t1->kind<MODIFIERS_END && t2->kind<MODIFIERS_END);
    b=t1->kind; modif=t2->kind;// just to confuse compiler warning
    /* if both are types, error, return the new one only*/
    if (t1->kind <= MODIFIERS_START && t2->kind <= MODIFIERS_START) return(t2);
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
    return(createSimpleTypeModifier(r));
}

static TypeModifier * mergeBaseModTypes(TypeModifier *t1, TypeModifier *t2) {
    assert(t1 && t2);
    if (t1->kind == TypeDefault) return(t2);
    if (t2->kind == TypeDefault) return(t1);
    assert(t1->kind >=0 && t1->kind<MAX_TYPE);
    assert(t2->kind >=0 && t2->kind<MAX_TYPE);
    if (s_preCreatedTypesTable[t2->kind] == NULL) return(t2);  /* not base type*/
    if (s_preCreatedTypesTable[t1->kind] == NULL) return(t1);  /* not base type*/
    return(mergeBaseType(t1, t2));
}

Symbol *typeSpecifier2(TypeModifier *t) {
    Symbol    *r;

    /* this is temporary, as long as we do not have the tempmemory in java, c++ */
    if (LANGUAGE(LANG_C)) {
        SM_ALLOC(tmpWorkMemory, r, Symbol);
    } else {
        XX_ALLOC(r, Symbol);
    }
    fillSymbolWithType(r, NULL, NULL, s_noPos, t);

    return r;
}

Symbol *typeSpecifier1(unsigned t) {
    Symbol        *r;
    r = typeSpecifier2(createSimpleTypeModifier(t));
    return(r);
}

void declTypeSpecifier1(Symbol *d, Type type) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(d->u.type,createSimpleTypeModifier(type));
}

void declTypeSpecifier2(Symbol *d, TypeModifier *t) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(d->u.type, t);
}

void declTypeSpecifier21(TypeModifier *t, Symbol *d) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(t, d->u.type);
}

TypeModifier *appendComposedType(TypeModifier **d, Type type) {
    TypeModifier *p;
    p = newTypeModifier(type, NULL, NULL);
    LIST_APPEND(TypeModifier, (*d), p);
    return(p);
}

TypeModifier *prependComposedType(TypeModifier *d, Type type) {
    return newTypeModifier(type, NULL, d);
}

void completeDeclarator(Symbol *t, Symbol *d) {
    TypeModifier *tt,**dt;
    //static int counter=0;
    assert(t && d);
    if (t == &s_errorSymbol || d == &s_errorSymbol
        || t->bits.symType==TypeError || d->bits.symType==TypeError) return;
    d->bits.storage = t->bits.storage;
    assert(t->bits.symType==TypeDefault);
    dt = &(d->u.type); tt = t->u.type;
    if (d->bits.npointers) {
        if (d->bits.npointers>=1 && (tt->kind==TypeStruct||tt->kind==TypeUnion)
            && tt->typedefSymbol==NULL) {
            //fprintf(dumpOut,"saving 1 str pointer:%d\n",counter++);fflush(dumpOut);
            d->bits.npointers--;
            //if(d->b.npointers) {fprintf(dumpOut,"possible 2\n");fflush(dumpOut);}
            assert(tt->u.t && tt->u.t->bits.symType==tt->kind && tt->u.t->u.s);
            tt = & tt->u.t->u.s->sptrtype;
        } else if (d->bits.npointers>=2 && s_preCrPtr2TypesTab[tt->kind]!=NULL
                   && tt->typedefSymbol==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 2 pointer\n");fflush(dumpOut);
            d->bits.npointers-=2;
            tt = s_preCrPtr2TypesTab[tt->kind];
        } else if (d->bits.npointers>=1 && s_preCrPtr1TypesTab[tt->kind]!=NULL
                   && tt->typedefSymbol==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 1 pointer\n");fflush(dumpOut);
            d->bits.npointers--;
            tt = s_preCrPtr1TypesTab[tt->kind];
        }
    }
    unpackPointers(d);
    LIST_APPEND(TypeModifier, *dt, tt);
}

Symbol *createSimpleDefinition(Storage storage, Type type, Id *id) {
    TypeModifier *typeModifier;
    Symbol *r;

    typeModifier = newTypeModifier(type, NULL, NULL);
    if (id!=NULL) {
        r = newSymbolAsType(id->name, id->name, id->p, typeModifier);
    } else {
        r = newSymbolAsType(NULL, NULL, s_noPos, typeModifier);
    }
    fillSymbolBits(&r->bits, AccessDefault, TypeDefault, storage);

    return r;
}

SymbolList *createDefinitionList(Symbol *symbol) {
    SymbolList *p;

    assert(symbol);
    p = StackMemAlloc(SymbolList);
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
                if (p->u.type == NULL) p->u.type = ty->u.type;
            }
        }
    }
    return(res);
}

static TypeModifier *createSimpleEnumType(Symbol *enumDefinition) {
    return newEnumTypeModifier(enumDefinition);
}

void initSymStructSpec(S_symStructSpec *symStruct, Symbol *records) {
    memset((void*)symStruct, 0, sizeof(*symStruct));
    symStruct->records = records;
    symStruct->classFile = -1;  /* Should be s_noFile? */
}

TypeModifier *simpleStrUnionSpecifier(Id *typeName,
                                        Id *id,
                                        int usage
                                        ) {
    Symbol p,*pp;
    int ii,type;

    log_trace("new struct %s", id->name);
    assert(typeName && typeName->symbol && typeName->symbol->bits.symType == TypeKeyword);
    assert(     typeName->symbol->u.keyWordVal == STRUCT
                ||  typeName->symbol->u.keyWordVal == CLASS
                ||  typeName->symbol->u.keyWordVal == UNION
                );
    if (typeName->symbol->u.keyWordVal != UNION) type = TypeStruct;
    else type = TypeUnion;

    fillSymbol(&p, id->name, id->name, id->p);
    fillSymbolBits(&p.bits, AccessDefault, type, StorageNone);

    if (! symbolTableIsMember(s_symbolTable,&p,&ii,&pp)
        || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        //{static int c=0;fprintf(dumpOut,"str#%d\n",c++);}
        XX_ALLOC(pp, Symbol);
        *pp = p;
        XX_ALLOC(pp->u.s, S_symStructSpec);

        initSymStructSpec(pp->u.s, /*.records=*/NULL);
        TypeModifier *stype = &pp->u.s->stype;
        /* Assumed to be Struct/Union/Enum? */
        initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/pp,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
        TypeModifier *sptrtype = &pp->u.s->sptrtype;
        initTypeModifierAsPointer(sptrtype, &pp->u.s->stype);

        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbol(pp, s_symbolTable);
    }
    addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
    return(&pp->u.s->stype);
}

void setGlobalFileDepNames(char *iname, Symbol *pp, int memory) {
    char            *mname, *fname;
    char            tmp[MACRO_NAME_SIZE];
    Symbol        *memb;
    int             ii,rr,filen, order, len, len2;
    if (iname == NULL) iname="";
    assert(pp);
    if (options.exactPositionResolve) {
        fname = simpleFileName(fileTable.tab[pp->pos.file]->name);
        sprintf(tmp, "%x-%s-%x-%x%c",
                hashFun(fileTable.tab[pp->pos.file]->name),
                fname, pp->pos.line, pp->pos.col,
                LINK_NAME_SEPARATOR);
    } else if (iname[0]==0) {
        // anonymous enum/structure/union ...
        filen = pp->pos.file;
        pp->name=iname; pp->linkName=iname;
        order = 0;
        rr = symbolTableIsMember(s_symbolTable, pp, &ii, &memb);
        while (rr) {
            if (memb->pos.file==filen) order++;
            rr = symbolTableNextMember(pp, &memb);
        }
        fname = simpleFileName(fileTable.tab[filen]->name);
        sprintf(tmp, "%s%c%d%c", fname, FILE_PATH_SEPARATOR, order, LINK_NAME_SEPARATOR);
        /*&     // macros will be identified by name only?
          } else if (pp->bits.symType == TypeMacro) {
          sprintf(tmp, "%x%c", pp->pos.file, LINK_NAME_SEPARATOR);
          &*/
    } else {
        tmp[0] = 0;
    }
    len = strlen(tmp);
    len2 = len + strlen(iname);
    assert(len < MACRO_NAME_SIZE-2);
    if (memory == MEMORY_XX) {
        XX_ALLOCC(mname, len2+1, char);
    } else {
        PP_ALLOCC(mname, len2+1, char);
    }
    strcpy(mname, tmp);
    strcpy(mname+len,iname);
    pp->name = mname + len;
    pp->linkName = mname;
}

TypeModifier *createNewAnonymousStructOrUnion(Id *typeName) {
    Symbol *pp;
    int type;

    assert(typeName);
    assert(typeName->symbol);
    assert(typeName->symbol->bits.symType == TypeKeyword);
    assert(typeName->symbol->u.keyWordVal == STRUCT
           ||  typeName->symbol->u.keyWordVal == CLASS
           ||  typeName->symbol->u.keyWordVal == UNION
           );
    if (typeName->symbol->u.keyWordVal == STRUCT) type = TypeStruct;
    else type = TypeUnion;

    pp = newSymbol("", NULL, typeName->p);
    fillSymbolBits(&pp->bits, AccessDefault, type, StorageNone);

    setGlobalFileDepNames("", pp, MEMORY_XX);

    XX_ALLOC(pp->u.s, S_symStructSpec);

    /* This is a recurring pattern, create a struct and the pointer type to it*/
    initSymStructSpec(pp->u.s, /*.records=*/NULL);
    TypeModifier *stype = &pp->u.s->stype;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/pp,
                                        /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *sptrtype = &pp->u.s->sptrtype;
    initTypeModifierAsPointer(sptrtype, &pp->u.s->stype);

    addSymbol(pp, s_symbolTable);

    return(&pp->u.s->stype);
}

void specializeStrUnionDef(Symbol *sd, Symbol *rec) {
    Symbol *dd;
    assert(sd->bits.symType == TypeStruct || sd->bits.symType == TypeUnion);
    assert(sd->u.s);
    if (sd->u.s->records!=NULL) return;
    sd->u.s->records = rec;
    addToTrail(setToNull, & (sd->u.s->records) );
    for(dd=rec; dd!=NULL; dd=dd->next) {
        if (dd->name!=NULL) {
            dd->linkName = string3ConcatInStackMem(sd->linkName,".",dd->name);
            dd->bits.isRecord = 1;
            addCxReference(dd,&dd->pos,UsageDefined,s_noneFileIndex, s_noneFileIndex);
        }
    }
}

TypeModifier *simpleEnumSpecifier(Id *id, int usage) {
    Symbol p,*pp;
    int ii;

    fillSymbol(&p, id->name, id->name, id->p);
    fillSymbolBits(&p.bits, AccessDefault, TypeEnum, StorageNone);

    if (! symbolTableIsMember(s_symbolTable,&p,&ii,&pp)
        || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        pp = StackMemAlloc(Symbol);
        *pp = p;
        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbol(pp, s_symbolTable);
    }
    addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
    return(createSimpleEnumType(pp));
}

TypeModifier *createNewAnonymousEnum(SymbolList *enums) {
    Symbol *pp;

    pp = newSymbolAsEnum("", "", s_noPos, enums);
    fillSymbolBits(&pp->bits, AccessDefault, TypeEnum, StorageNone);

    setGlobalFileDepNames("", pp, MEMORY_XX);
    pp->u.enums = enums;
    return(createSimpleEnumType(pp));
}

void appendPositionToList( PositionList **list,Position *pos) {
    PositionList *ppl;
    XX_ALLOC(ppl, PositionList);
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
    return newSymbolAsType("", "", s_noPos, p);
}

void handleDeclaratorParamPositions(Symbol *decl, Position *lpar,
                                    PositionList *commas, Position *rpar,
                                    int hasParam
                                    ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (POSITION_NEQ(decl->pos, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void handleInvocationParamPositions(Reference *ref, Position *lpar,
                                    PositionList *commas, Position *rpar,
                                    int hasParam
                                    ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (ref==NULL || POSITION_NEQ(ref->p, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void javaHandleDeclaratorParamPositions(Position *sym, Position *lpar,
                                        PositionList *commas, Position *rpar
                                        ) {
    if (options.taskRegime != RegimeEditServer) return;
    if (options.server_operation != OLO_GOTO_PARAM_NAME && options.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (POSITION_NEQ(*sym, s_cxRefPos)) return;
    if (commas==NULL) {
        handleParameterPositions(lpar, NULL, rpar, 0);
    } else {
        handleParameterPositions(lpar, commas->next, rpar, 1);
    }
}
