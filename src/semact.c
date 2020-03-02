#include "semact.h"

#include "commons.h"
#include "globals.h"
#include "parsers.h"
#include "misc.h"
#include "cct.h"
#include "yylex.h"
#include "cxref.h"
#include "jsemact.h"
#include "jslsemact.h"
#include "enumTxt.h"
#include "symbol.h"
#include "list.h"
#include "strFill.h"

#include "hash.h"
#include "log.h"


int displayingErrorMessages(void) {
    // no error messages for file preloaded for symbols
    if (LANGUAGE(LANG_JAVA) && s_jsl!=NULL) return(0);
    if (s_opt.debug || s_opt.err) return(1);
    return(0);
}

int styyerror(char *s) {
    if (strcmp(s,"syntax error")!=0) {
        sprintf(tmpBuff,"YACC error: %s",s);
        error(ERR_INTERNAL,tmpBuff);
    }
    if (displayingErrorMessages()) {
        sprintf(tmpBuff,"on: %s",yytext);
        error(ERR_ST, tmpBuff);
    }
    return(0);
}

int styyErrorRecovery(void) {
    if (s_opt.debug && displayingErrorMessages()) {
        error(ERR_ST, " recovery");
    }
    return(0);
}

void setToNull(void *p) {
    void **pp;
    pp = (void **)p;
    *pp = NULL;
}

void deleteSymDef(void *p) {
    S_symbol        *pp;

    pp = (S_symbol *) p;
    log_debug("deleting %s %s", pp->name, pp->linkName);
    if (symTabDelete(s_javaStat->locals,pp)) return;
    if (symTabDelete(s_symTab,pp)==0) {
        assert(s_opt.taskRegime);
        if (s_opt.taskRegime != RegimeEditServer) {
            error(ERR_INTERNAL,"symbol on deletion not found");
        }
    }
}

void unpackPointers(S_symbol *pp) {
    unsigned i;
    for (i=0; i<pp->bits.npointers; i++) {
        appendComposedType(&pp->u.type, TypePointer);
    }
    pp->bits.npointers=0;
}

void addSymbol(S_symbol *pp, S_symTab *tab) {
    /*  a bug can produce, if you add a symbol into old table, and the same
        symbol exists in a newer one. Then it will be deleted from the newer
        one. All this story is about storing information in trail. It should
        containt, both table and pointer !!!!
    */
    log_debug("adding symbol %s: %s %s",pp->name, typesName[pp->bits.symType], storagesName[pp->bits.storage]);
    assert(pp->bits.npointers==0);
    AddSymbolNoTrail(pp,tab);
    addToTrail(deleteSymDef, pp  /* AND ALSO!!! , tab */ );
    //if (WORK_NEST_LEVEL0()) {static int c=0;fprintf(dumpOut,"addsym0#%d\n",c++);}
}

void recFindPush(S_symbol *str, S_recFindStr *rfs) {
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

S_recFindStr * iniFind(S_symbol *s, S_recFindStr *rfs) {
    assert(s);
    assert(s->bits.symType == TypeStruct || s->bits.symType == TypeUnion);
    assert(s->u.s);
    assert(rfs);
    FILL_recFindStr(rfs, s, NULL, NULL,s_recFindCl++, 0, 0);
    recFindPush(s, rfs);
    return(rfs);
}

#if ZERO //DEBUG_ACCESS
#define DAP(xxx) xxx
#else
#define DAP(xxx)
#endif

int javaOuterClassAccessible(S_symbol *cl) {
    log_trace("testing class accessibility of %s",cl->linkName);
    if (cl->bits.accessFlags & ACC_PUBLIC) {
        log_trace("ret 1 access public");
        return 1;
    }
    /* default access, check whether it is in current package */
    assert(s_javaStat);
    if (javaClassIsInCurrentPackage(cl)) {
        log_trace("ret 1 default protection in current package");
        return 1;
    }
    log_trace("ret 0 on default");
    return 0;

}

static int javaRecordVisible(S_symbol *appcl, S_symbol *funcl, unsigned accessFlags) {
    // there is special case to check! Private symbols are not inherited!
    if (accessFlags & ACC_PRIVATE) {
        // check classes to string equality, just to be sure
        if (appcl!=funcl && strcmp(appcl->linkName, funcl->linkName)!=0) return(0);
    }
    return(1);
}

static int accessibleByDefaultAccessibility(S_recFindStr *rfs, S_symbol *funcl) {
    int             i;
    S_symbol        *cc;
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
int javaRecordAccessible(S_recFindStr *rfs, S_symbol *appcl, S_symbol *funcl, S_symbol *rec, unsigned recAccessFlags) {
    S_javaStat          *cs, *lcs;
    int                 len;
    if (funcl == NULL) return(1);  /* argument or local variable */
    log_trace("testing accessibility %s . %s of x%x",funcl->linkName,rec->linkName, recAccessFlags);
    assert(s_javaStat);
    if (recAccessFlags & ACC_PUBLIC) {
        log_trace("ret 1 access public");
        return 1;
    }
    if (recAccessFlags & ACC_PROTECTED) {
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
    if (recAccessFlags & ACC_PRIVATE) {
        // finally it seems that following is wrong and that private field
        // can be accessed from all classes within same major class
#if ZERO
        for(cs=s_javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            if (cs->thisClass == funcl) return(1);
        }
#endif
        // I thinked it was wrong, but it seems that it is definitely O.K.
        // it seems that if cl is defined inside top class, than it is O.K.
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

int javaRecordVisibleAndAccessible(S_recFindStr *rfs, S_symbol *applCl, S_symbol *funCl, S_symbol *r) {
    return(
           javaRecordVisible(rfs->baseClass, rfs->currClass, r->bits.accessFlags)
           &&
           javaRecordAccessible(rfs, rfs->baseClass, rfs->currClass, r, r->bits.accessFlags)
           );
}

int javaGetMinimalAccessibility(S_recFindStr *rfs, S_symbol *r) {
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

int findStrRecordSym(   S_recFindStr    *ss,
                        char            *recname,    /* can be NULL */
                        S_symbol        **res,
                        int             javaClassif, /* classify to method/field*/
                        int             accCheck,    /* java check accessibility */
                        int             visibilityCheck /* redundant, always equal to accCheck? */
                        ) {
    S_symbol            *s,*r,*cclass;
    SymbolList        *sss;
    int                 m;

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
                && r->u.type->kind==TypeAnonymeField
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
                //&if(cclass!=NULL)fprintf(dumpOut,"name O.K., checking accesibility %xd %xd\n",cclass->bits.accessFlags,r->bits.accessFlags); fflush(dumpOut);
                // I have it, check visibility and accessibility
                assert(r);
                if (visibilityCheck == VISIB_CHECK_YES) {
                    if (! javaRecordVisible(ss->baseClass, cclass, r->bits.accessFlags)) {
                        // WRONG? return, Doesn't it iverrides any other of this name
                        // Yes, definitely correct, in the first step determining
                        // class to search
                        FSRS_RETURN_WITH_FAIL(ss, res);
                    }
                }
                if (accCheck == ACC_CHECK_YES) {
                    if (! javaRecordAccessible(ss, ss->baseClass,cclass,r,r->bits.accessFlags)){
                        if (visibilityCheck == VISIB_CHECK_YES) {
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

int findStrRecord(  S_symbol        *s,
                    char            *recname,   /* can be NULL */
                    S_symbol        **res,
                    int             javaClassif
                    ) {
    S_recFindStr rfs;
    return(findStrRecordSym(iniFind(s,&rfs),recname,res,javaClassif,
                            ACC_CHECK_YES,VISIB_CHECK_YES));
}

/* and push reference */
// this should be split into two copies, different for C and Java.
S_reference *findStrRecordFromSymbol( S_symbol *sym,
                                      S_idIdent *record,
                                      S_symbol **res,
                                      int javaClassif,
                                      S_idIdent *super /* covering special case when invoked
                                                          as SUPER.sym, berk */
                                      ) {
    S_recFindStr    rfs;
    S_reference     *ref;
    S_usageBits     ub;
    int rr, minacc;
    ref = NULL;
    // when in java, then always in qualified name, so access and visibility checks
    // are useless.
    rr = findStrRecordSym(iniFind(sym,&rfs),record->name,res,
                          javaClassif, ACC_CHECK_NO, VISIB_CHECK_NO);
    if (rr == RESULT_OK && rfs.currClass!=NULL &&
        ((*res)->bits.storage==StorageField
         || (*res)->bits.storage==StorageMethod
         || (*res)->bits.storage==StorageConstructor)){
        assert(rfs.currClass->u.s && rfs.baseClass && rfs.baseClass->u.s);
        if ((s_opt.ooChecksBits & OOC_ALL_CHECKS)==0
            || javaRecordVisibleAndAccessible(&rfs, rfs.baseClass, rfs.currClass, *res)) {
            minacc = javaGetMinimalAccessibility(&rfs, *res);
            FILL_usageBits(&ub, UsageUsed, minacc, 0);
            ref = addCxReferenceNew(*res,&record->p, &ub,
                                    rfs.currClass->u.s->classFile,
                                    rfs.baseClass->u.s->classFile);
            // this is adding reference to 'super', not to the field!
            // for pull-up/push-down
            if (super!=NULL) addThisCxReferences(s_javaStat->classFileInd,&super->p);
        }
    } else if (rr == RESULT_OK) {
        ref = addCxReference(*res,&record->p,UsageUsed, s_noneFileIndex, s_noneFileIndex);
    } else {
        noSuchRecordError(record->name);
    }
    return(ref);
}

S_reference * findStrRecordFromType(    S_typeModifiers *str,
                                        S_idIdent *record,
                                        S_symbol **res,
                                        int javaClassif
                                        ) {
    S_reference *ref;
    assert(str);
    ref = NULL;
    if (str->kind != TypeStruct && str->kind != TypeUnion) {
        *res = &s_errorSymbol;
        goto fini;
    }
    ref = findStrRecordFromSymbol( str->u.t, record, res, javaClassif, NULL);
 fini:
    return(ref);
}

void labelReference(S_idIdent *id, int usage) {
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
    if (s_opt.server_operation == OLO_EXTRACT) {
        // extract variable, I must pass all needed informations in linkname
        sprintf(nnn,"%c%s%c",   LINK_NAME_CUT_SYMBOL, p->name,
                LINK_NAME_CUT_SYMBOL);
        ttt[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
        // why it commented out ?
        //& if ((!LANGUAGE(LANG_JAVA))
        //&     && (p->u.type->kind == TypeUnion || p->u.type->kind == TypeStruct)) {
        //&     ttt[0] = LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
        //& }
        sprintf(ttt+1,"%s", s_extractStorageName[p->bits.storage]);
        tti = strlen(ttt);
        len = TMP_STRING_SIZE - tti;
        typeSPrint(ttt+tti, &len, p->u.type, nnn, LINK_NAME_CUT_SYMBOL, 0,1,SHORT_NAME, NULL);
        sprintf(ttt+tti+len,"%c%x-%x-%x-%x", LINK_NAME_CUT_SYMBOL,
                p->pos.file,p->pos.line,p->pos.col, s_count.localVar++);
    } else {
        if (p->bits.storage==StorageExtern && ! s_opt.exactPositionResolve) {
            sprintf(ttt,"%s", p->name);
        } else {
            // it is now better to have name allways accessible
            //&         if (s_opt.taskRegime == RegimeHtmlGenerate) {
            // html symbol, must pass the name for cxreference list item
            sprintf(ttt,"%x-%x-%x%c%s",p->pos.file,p->pos.line,p->pos.col,
                    LINK_NAME_CUT_SYMBOL, p->name);
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

static void setStaticFunctionLinkName( S_symbol *p, int usage ) {
    char        ttt[TMP_STRING_SIZE];
    int         len;
    char        *ss,*basefname;

    //& if (! symTabIsMember(s_symTab, p, &ii, &memb)) {
    // follwing unifies static symbols taken from the same header files.
    // Static symbols can be used only after being defined, so it is sufficient
    // to do this on definition usage?
    // With exactPositionResolve interpret them as distinct symbols for
    // each compilation unit.
    if (usage==UsageDefined && ! s_opt.exactPositionResolve) {
        basefname=s_fileTab.tab[p->pos.file]->name;
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

S_symbol *addNewSymbolDef(S_symbol *p, unsigned theDefaultStorage, S_symTab *tab,
                          int usage) {
    S_typeModifiers *tt;
    S_symbol *pp;
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
        XX_ALLOC(tt, S_typeModifiers);
        *tt = *p->u.type;
        p->u.type = tt;
        tt->typedefin = p;
    }
    if ((! WORK_NEST_LEVEL0() && LANGUAGE(LANG_C))
        || (! WORK_NEST_LEVEL1() && LANGUAGE(LAN_YACC))) {
        // local scope symbol
        if (! symTabIsMember(s_symTab,p,&ii,&pp)
            || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
            pp = p;
            setLocalVariableLinkName(pp);
            addSymbol(pp, tab);
        }
    } else if (p->bits.symType==TypeDefault && p->bits.storage==StorageStatic) {
        if (! symTabIsMember(s_symTab,p,&ii,&pp)) {
            pp = p;
            setStaticFunctionLinkName(pp, usage);
            addSymbol(pp, tab);
        }
    } else {
        if (! symTabIsMember(s_symTab,p,&ii,&pp)) {
            pp = p;
            if (s_opt.exactPositionResolve) {
                setGlobalFileDepNames(pp->name, pp, MEM_XX);
            }
            addSymbol(pp, tab);
        }
    }
    addCxReference(pp, &p->pos, usage,s_noneFileIndex, s_noneFileIndex);
    return(pp);
}

/* this function is dead man, nowhere used */
S_symbol *addNewCopyOfSymbolDef(S_symbol *def, unsigned storage) {
    S_symbol *p;
    p = StackMemAlloc(S_symbol);
    *p = *def;
    addNewSymbolDef(p,storage, s_symTab, UsageDefined);
    return(p);
}

void addInitializerRefs(
                        S_symbol *decl,
                        S_idIdentList *idl
                        ) {
    S_idIdentList *ll;
    S_idIdent* id;
    S_typeModifiers *tt;
    S_reference *ref;
    S_symbol *rec=NULL;
    for(ll=idl; ll!=NULL; ll=ll->next) {
        tt = decl->u.type;
        for (id = &ll->idi; id!=NULL; id=id->next) {
            if (tt->kind == TypeArray) {
                tt = tt->next;
                continue;
            }
            if (tt->kind != TypeStruct && tt->kind != TypeUnion) return;
            ref = findStrRecordFromType(tt, id, &rec, CLASS_TO_ANY);
            if (NULL == ref) return;
            assert(rec);
            tt = rec->u.type;
        }
    }
}

S_symbol *addNewDeclaration(
                            S_symbol *btype,
                            S_symbol *decl,
                            S_idIdentList *idl,
                            unsigned storage,
                            S_symTab *tab
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

void addFunctionParameterToSymTable(S_symbol *function, S_symbol *p, int i, S_symTab *tab) {
    S_symbol    *pp, *pa, *ppp;
    int         ii;
    if (p->name != NULL && p->bits.symType!=TypeError) {
        assert(s_javaStat->locals!=NULL);
        XX_ALLOC(pa, S_symbol);
        *pa = *p;
        // here checks a special case, double argument definition do not
        // redefine him, so refactorings will detect problem
        for(pp=function->u.type->u.f.args; pp!=NULL && pp!=p; pp=pp->next) {
            if (pp->name!=NULL && pp->bits.symType!=TypeError) {
                if (p!=pp && strcmp(pp->name, p->name)==0) break;
            }
        }
        if (pp!=NULL && pp!=p) {
            if (symTabIsMember(tab, pa, &ii, &ppp)) {
                addCxReference(ppp, &p->pos, UsageUsed, s_noneFileIndex, s_noneFileIndex);
            }
        } else {
            addNewSymbolDef(pa, StorageAuto, tab, UsageDefined);
        }
        if (s_opt.server_operation == OLO_EXTRACT) {
            addCxReference(pa, &pa->pos, UsageLvalUsed,
                           s_noneFileIndex, s_noneFileIndex);
        }
    }
    if (s_opt.server_operation == OLO_GOTO_PARAM_NAME
        && i == s_opt.olcxGotoVal
        && POSITION_EQ(function->pos, s_cxRefPos)) {
        s_paramPosition = p->pos;
    }
}

S_typeModifiers *crSimpleTypeModifier(unsigned t) {
    S_typeModifiers *p;
    assert(t>=0 && t<MAX_TYPE);
    if (s_preCrTypesTab[t] == NULL) {
        p = StackMemAlloc(S_typeModifiers);
        FILLF_typeModifiers(p,t,f,( NULL,NULL) ,NULL,NULL);
    } else {
        p = s_preCrTypesTab[t];
    }
    /*fprintf(dumpOut,"t,p->m == %d %d == %s %s\n",t,p->m,typesName[t],typesName[p->m]); fflush(dumpOut);*/
    assert(p->kind == t);
    return(p);
}

static S_typeModifiers *mergeBaseType(S_typeModifiers *t1,S_typeModifiers *t2){
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
    return(crSimpleTypeModifier(r));
}

static S_typeModifiers * mergeBaseModTypes(S_typeModifiers *t1, S_typeModifiers *t2) {
    assert(t1 && t2);
    if (t1->kind == TypeDefault) return(t2);
    if (t2->kind == TypeDefault) return(t1);
    assert(t1->kind >=0 && t1->kind<MAX_TYPE);
    assert(t2->kind >=0 && t2->kind<MAX_TYPE);
    if (s_preCrTypesTab[t2->kind] == NULL) return(t2);  /* not base type*/
    if (s_preCrTypesTab[t1->kind] == NULL) return(t1);  /* not base type*/
    return(mergeBaseType(t1, t2));
}

S_symbol *typeSpecifier2(S_typeModifiers *t) {
    S_symbol    *r;
    /* this is temporary, as long as we do not have the tempmemory in java, c++ */
    if (LANGUAGE(LANG_C)) {
        SM_ALLOC(tmpWorkMemory, r, S_symbol);
    } else {
        XX_ALLOC(r, S_symbol);
    }
    fillSymbolWithType(r, NULL, NULL, s_noPos, t);
    FILL_symbolBits(&r->bits,0,0,0,0,0,TypeDefault,StorageDefault,0);

    return(r);
}

S_symbol *typeSpecifier1(unsigned t) {
    S_symbol        *r;
    r = typeSpecifier2(crSimpleTypeModifier(t));
    return(r);
}

void declTypeSpecifier1(S_symbol *d, unsigned t) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(d->u.type,crSimpleTypeModifier(t));
}

void declTypeSpecifier2(S_symbol *d, S_typeModifiers *t) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(d->u.type, t);
}

void declTypeSpecifier21(S_typeModifiers *t, S_symbol *d) {
    assert(d && d->u.type);
    d->u.type = mergeBaseModTypes(t, d->u.type);
}

S_typeModifiers *appendComposedType(S_typeModifiers **d, unsigned t) {
    S_typeModifiers *p;
    p = StackMemAlloc(S_typeModifiers);
    FILLF_typeModifiers(p, t,f,( NULL,NULL) ,NULL,NULL);
    LIST_APPEND(S_typeModifiers, (*d), p);
    return(p);
}

S_typeModifiers *prependComposedType(S_typeModifiers *d, unsigned t) {
    S_typeModifiers *p;
    p = StackMemAlloc(S_typeModifiers);
    FILLF_typeModifiers(p, t,f,( NULL,NULL) ,NULL,d);
    return(p);
}

#if ZERO
void completeDeclarator(S_symbol *t, S_symbol *d) {
    assert(t && d);
    if (t == &s_errorSymbol || d == &s_errorSymbol) return;
    unpackPointers(d);
    LIST_APPEND(S_typeModifiers, d->u.type, t->u.type);
    d->b.storage = t->b.storage;
}
#endif

void completeDeclarator(S_symbol *t, S_symbol *d) {
    S_typeModifiers *tt,**dt;
    //static int counter=0;
    assert(t && d);
    if (t == &s_errorSymbol || d == &s_errorSymbol
        || t->bits.symType==TypeError || d->bits.symType==TypeError) return;
    d->bits.storage = t->bits.storage;
    assert(t->bits.symType==TypeDefault);
    dt = &(d->u.type); tt = t->u.type;
    if (d->bits.npointers) {
        if (d->bits.npointers>=1 && (tt->kind==TypeStruct||tt->kind==TypeUnion)
            && tt->typedefin==NULL) {
            //fprintf(dumpOut,"saving 1 str pointer:%d\n",counter++);fflush(dumpOut);
            d->bits.npointers--;
            //if(d->b.npointers) {fprintf(dumpOut,"possible 2\n");fflush(dumpOut);}
            assert(tt->u.t && tt->u.t->bits.symType==tt->kind && tt->u.t->u.s);
            tt = & tt->u.t->u.s->sptrtype;
        } else if (d->bits.npointers>=2 && s_preCrPtr2TypesTab[tt->kind]!=NULL
                   && tt->typedefin==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 2 pointer\n");fflush(dumpOut);
            d->bits.npointers-=2;
            tt = s_preCrPtr2TypesTab[tt->kind];
        } else if (d->bits.npointers>=1 && s_preCrPtr1TypesTab[tt->kind]!=NULL
                   && tt->typedefin==NULL) {
            assert(tt->next==NULL); /* not a user defined type */
            //fprintf(dumpOut,"saving 1 pointer\n");fflush(dumpOut);
            d->bits.npointers--;
            tt = s_preCrPtr1TypesTab[tt->kind];
        }
    }
    unpackPointers(d);
    LIST_APPEND(S_typeModifiers, *dt, tt);
}

S_symbol *createSimpleDefinition(unsigned storage, unsigned t, S_idIdent *id) {
    S_typeModifiers *typeModifiers;
    S_symbol *r;
    typeModifiers = StackMemAlloc(S_typeModifiers);
    FILLF_typeModifiers(typeModifiers,t,f,( NULL,NULL) ,NULL,NULL);
    if (id!=NULL) {
        /*& r = StackMemAlloc(S_symbol); */
        /*& FILL_symbolBits(&r->bits,0,0,0,0,0,TypeDefault,storage,0); */
        /*& FILL_symbol(r,id->name,id->name,id->p,r->bits,type,typeModifiers,NULL); */
        /* REPLACED StackMemAlloc()+FILL_symbol() with */
        r = newSymbolIsType(id->name, id->name, id->p, typeModifiers);
    } else {
        /*& r = StackMemAlloc(S_symbol); */
        /*& FILL_symbolBits(&r->bits,0,0,0,0,0,TypeDefault,storage,0); */
        /*& FILL_symbol(r,NULL, NULL, s_noPos,r->bits,type,typeModifiers,NULL); */
        r = newSymbolIsType(NULL, NULL, s_noPos, typeModifiers);
    }
    FILL_symbolBits(&r->bits, 0, 0, 0, 0, 0, TypeDefault, storage, 0);
    return(r);
}

SymbolList *createDefinitionList(S_symbol *symbol) {
    SymbolList *p;

    assert(symbol);
    p = StackMemAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(p, symbol, NULL); with: */
    *p = (SymbolList){.d = symbol, .next = NULL};

    return p;
}

int mergeArguments(S_symbol *id, S_symbol *ty) {
    S_symbol *p;
    int res;
    res = RESULT_OK;
    /* if a type of non-exist. argument is declared, it is probably */
    /* only a missing ';', so syntax error should be raised */
    for(;ty!=NULL; ty=ty->next) {
        if (ty->name != NULL) {
            for(p=id; p!=NULL; p=p->next) {
                if (p->name!=NULL && !strcmp(p->name,ty->name)) break;
            }
            if (p==NULL) res = RESULT_ERR;
            else {
                if (p->u.type == NULL) p->u.type = ty->u.type;
            }
        }
    }
    return(res);
}

static S_typeModifiers *crSimpleEnumType(S_symbol *edef, int type) {
    S_typeModifiers *res;
    res = StackMemAlloc(S_typeModifiers);
    FILLF_typeModifiers(res, type,t,edef,NULL,NULL);
    res->u.t = edef;
    return(res);
}

S_typeModifiers *simpleStrUnionSpecifier(   S_idIdent *typeName,
                                            S_idIdent *id,
                                            int usage
                                            ) {
    S_symbol p,*pp;
    int ii,type;
    /*fprintf(dumpOut, "new str %s\n",id->name); fflush(dumpOut);*/
    assert(typeName && typeName->sd && typeName->sd->bits.symType == TypeKeyword);
    assert(     typeName->sd->u.keyWordVal == STRUCT
                ||  typeName->sd->u.keyWordVal == CLASS
                ||  typeName->sd->u.keyWordVal == UNION
                );
    if (typeName->sd->u.keyWordVal != UNION) type = TypeStruct;
    else type = TypeUnion;

    fillSymbol(&p, id->name, id->name, id->p);
    FILL_symbolBits(&p.bits, 0, 0, 0, 0, 0, type, StorageNone, 0);

    if (! symTabIsMember(s_symTab,&p,&ii,&pp)
        || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        //{static int c=0;fprintf(dumpOut,"str#%d\n",c++);}
        XX_ALLOC(pp, S_symbol);
        *pp = p;
        XX_ALLOC(pp->u.s, S_symStructSpec);
        FILLF_symStructSpec(pp->u.s, NULL,
                            NULL,NULL,NULL,0,NULL,
                            type,f,(NULL,NULL),NULL,NULL,
                            TypePointer,f,(NULL,NULL),NULL,&pp->u.s->stype,
                            0,0, -1,0);
        pp->u.s->stype.u.t = pp;
        setGlobalFileDepNames(id->name, pp, MEM_XX);
        addSymbol(pp, s_symTab);
    }
    addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
    return(&pp->u.s->stype);
}

void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory) {
    char            *mname, *fname;
    char            tmp[MACRO_NAME_SIZE];
    S_symbol        *memb;
    int             ii,rr,filen, order, len, len2;
    if (iname == NULL) iname="";
    assert(pp);
    if (s_opt.exactPositionResolve) {
        fname = simpleFileName(s_fileTab.tab[pp->pos.file]->name);
        sprintf(tmp, "%x-%s-%x-%x%c",
                hashFun(s_fileTab.tab[pp->pos.file]->name),
                fname, pp->pos.line, pp->pos.col,
                LINK_NAME_CUT_SYMBOL);
    } else if (iname[0]==0) {
        // anonymous enum/structure/union ...
        filen = pp->pos.file;
        pp->name=iname; pp->linkName=iname;
        order = 0;
        rr = symTabIsMember(s_symTab, pp, &ii, &memb);
        while (rr) {
            if (memb->pos.file==filen) order++;
            rr = symTabNextMember(pp, &memb);
        }
        fname = simpleFileName(s_fileTab.tab[filen]->name);
        sprintf(tmp, "%s%c%d%c", fname, SLASH, order, LINK_NAME_CUT_SYMBOL);
        /*&     // macros will be identified by name only?
          } else if (pp->bits.symType == TypeMacro) {
          sprintf(tmp, "%x%c", pp->pos.file, LINK_NAME_CUT_SYMBOL);
          &*/
    } else {
        tmp[0] = 0;
    }
    len = strlen(tmp);
    len2 = len + strlen(iname);
    assert(len < MACRO_NAME_SIZE-2);
    if (memory == MEM_XX) {
        XX_ALLOCC(mname, len2+1, char);
    } else {
        PP_ALLOCC(mname, len2+1, char);
    }
    strcpy(mname, tmp);
    strcpy(mname+len,iname);
    pp->name = mname + len;
    pp->linkName = mname;
}

#if ZERO
void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory) {
    char            *mname, *fname;
    char            tmp[MACRO_NAME_SIZE];
    S_symbol        *memb;
    int             ii,rr, order, len, len2;
    if (iname == NULL) iname="";

    assert(pp);
    sprintf(tmp, "%x-%x%c", pp->pos.file, pp->pos.line, LINK_NAME_CUT_SYMBOL);

    len = strlen(tmp);
    len2 = len + strlen(iname);
    assert(len < MACRO_NAME_SIZE-2);
    if (memory == MEM_XX) {
        XX_ALLOCC(mname, len2+1, char);
    } else {
        PP_ALLOCC(mname, len2+1, char);
    }
    strcpy(mname, tmp);
    strcpy(mname+len,iname);
    pp->name = mname + len;
    pp->linkName = mname;
}
#endif

S_typeModifiers *crNewAnnonymeStrUnion(S_idIdent *typeName) {
    S_symbol *pp;
    int type;

    assert(typeName);
    assert(typeName->sd);
    assert(typeName->sd->bits.symType == TypeKeyword);
    assert(     typeName->sd->u.keyWordVal == STRUCT
                ||  typeName->sd->u.keyWordVal == CLASS
                ||  typeName->sd->u.keyWordVal == UNION
                );
    if (typeName->sd->u.keyWordVal == STRUCT) type = TypeStruct;
    else type = TypeUnion;

    /*& pp = StackMemAlloc(S_symbol); */
    /*& FILL_symbolBits(&pp->bits,0,0, 0,0,0, type, StorageNone,0); */
    /*& FILL_symbol(pp, "", NULL, typeName->p,pp->bits,type,NULL, NULL); */
    /*& REPLACED StackMemAlloc()+FILL_symbol() with: */
    pp = newSymbol("", NULL, typeName->p);
    FILL_symbolBits(&pp->bits, 0, 0, 0, 0, 0, type, StorageNone, 0);

    setGlobalFileDepNames("", pp, MEM_XX);
    XX_ALLOC(pp->u.s, S_symStructSpec);
    FILLF_symStructSpec(pp->u.s, NULL,
                        NULL, NULL, NULL, 0, NULL,
                        type,f,(NULL,NULL),NULL,NULL,
                        TypePointer,f,(NULL,NULL),NULL,&pp->u.s->stype,
                        0,0, -1,0);
    pp->u.s->stype.u.t = pp;
    addSymbol(pp, s_symTab);
    return(&pp->u.s->stype);
}

void specializeStrUnionDef(S_symbol *sd, S_symbol *rec) {
    S_symbol *dd;
    assert(sd->bits.symType == TypeStruct || sd->bits.symType == TypeUnion);
    assert(sd->u.s);
    if (sd->u.s->records!=NULL) return;
    sd->u.s->records = rec;
    addToTrail(setToNull, & (sd->u.s->records) );
    for(dd=rec; dd!=NULL; dd=dd->next) {
        if (dd->name!=NULL) {
            dd->linkName = string3ConcatInStackMem(sd->linkName,".",dd->name);
            dd->bits.record = 1;
            if (    LANGUAGE(LANG_CCC) && dd->bits.symType==TypeDefault
                    &&  dd->u.type->kind==TypeFunction) {
                dd->u.type->u.f.thisFunList = &sd->u.s->records;
            }
            addCxReference(dd,&dd->pos,UsageDefined,s_noneFileIndex, s_noneFileIndex);
        }
    }
}

S_typeModifiers *simpleEnumSpecifier(S_idIdent *id, int usage) {
    S_symbol p,*pp;
    int ii;

    fillSymbol(&p, id->name, id->name, id->p);
    FILL_symbolBits(&p.bits, 0, 0, 0, 0, 0, TypeEnum, StorageNone, 0);

    if (! symTabIsMember(s_symTab,&p,&ii,&pp)
        || (MEM_FROM_PREVIOUS_BLOCK(pp) && IS_DEFINITION_OR_DECL_USAGE(usage))) {
        pp = StackMemAlloc(S_symbol);
        *pp = p;
        setGlobalFileDepNames(id->name, pp, MEM_XX);
        addSymbol(pp, s_symTab);
    }
    addCxReference(pp, &id->p, usage,s_noneFileIndex, s_noneFileIndex);
    return(crSimpleEnumType(pp,TypeEnum));
}

S_typeModifiers *createNewAnonymousEnum(SymbolList *enums) {
    S_symbol *pp;

    /*& pp = StackMemAlloc(S_symbol); */
    /*& FILL_symbolBits(&pp->bits,0,0, 0,0,0, TypeEnum, StorageNone,0); */
    /*& FILL_symbol(pp, "", "", s_noPos,pp->bits,enums,enums, NULL); */
    /*& REPLACED StackMemAlloc()+FILL_symbol() with:  */
    pp = newSymbolIsEnum("", "", s_noPos, enums);
    FILL_symbolBits(&pp->bits, 0, 0, 0, 0, 0, TypeEnum, StorageNone, 0);

    setGlobalFileDepNames("", pp, MEM_XX);
    pp->u.enums = enums;
    return(crSimpleEnumType(pp,TypeEnum));
}

void appendPositionToList( S_positionList **list,S_position *pos) {
    S_positionList *ppl;
    XX_ALLOC(ppl, S_positionList);
    FILL_positionList(ppl, *pos, NULL);
    LIST_APPEND(S_positionList, (*list), ppl);
}

void setParamPositionForFunctionWithoutParams(S_position *lpar) {
    s_paramBeginPosition = *lpar;
    s_paramEndPosition = *lpar;
}

void setParamPositionForParameter0(S_position *lpar) {
    s_paramBeginPosition = *lpar;
    s_paramEndPosition = *lpar;
}

void setParamPositionForParameterBeyondRange(S_position *rpar) {
    s_paramBeginPosition = *rpar;
    s_paramEndPosition = *rpar;
}

static void handleParameterPositions(S_position *lpar, S_positionList *commas,
                                     S_position *rpar, int hasParam) {
    int i, argn;
    S_position *p1, *p2;
    S_positionList *pp;
    if (! hasParam) {
        setParamPositionForFunctionWithoutParams(lpar);
        return;
    }
    argn = s_opt.olcxGotoVal;
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

S_symbol *crEmptyField(void) {
    S_symbol *res;
    S_typeModifiers *p;

    p = StackMemAlloc(S_typeModifiers);
    FILLF_typeModifiers(p,TypeAnonymeField,f,( NULL,NULL) ,NULL,NULL);

    /*& res = StackMemAlloc(S_symbol); */
    /*& FILL_symbolBits(&res->bits,0,0,0,0,0,TypeDefault,StorageDefault,0); */
    /*& FILL_symbol(res, "", "", s_noPos,res->bits,type,p,NULL); */
    /*& REPLACED StackMemAlloc()+FILL_symbol() with  */
    res = newSymbolIsType("", "", s_noPos, p);
    FILL_symbolBits(&res->bits, 0, 0, 0, 0, 0, TypeDefault, StorageDefault, 0);

    return(res);
}

void handleDeclaratorParamPositions(S_symbol *decl, S_position *lpar,
                                    S_positionList *commas, S_position *rpar,
                                    int hasParam
                                    ) {
    if (s_opt.taskRegime != RegimeEditServer) return;
    if (s_opt.server_operation != OLO_GOTO_PARAM_NAME && s_opt.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (POSITION_NEQ(decl->pos, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void handleInvocationParamPositions(S_reference *ref, S_position *lpar,
                                    S_positionList *commas, S_position *rpar,
                                    int hasParam
                                    ) {
    if (s_opt.taskRegime != RegimeEditServer) return;
    if (s_opt.server_operation != OLO_GOTO_PARAM_NAME && s_opt.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (ref==NULL || POSITION_NEQ(ref->p, s_cxRefPos)) return;
    handleParameterPositions(lpar, commas, rpar, hasParam);
}

void javaHandleDeclaratorParamPositions(S_position *sym, S_position *lpar,
                                        S_positionList *commas, S_position *rpar
                                        ) {
    if (s_opt.taskRegime != RegimeEditServer) return;
    if (s_opt.server_operation != OLO_GOTO_PARAM_NAME && s_opt.server_operation != OLO_GET_PARAM_COORDINATES) return;
    if (POSITION_NEQ(*sym, s_cxRefPos)) return;
    if (commas==NULL) {
        handleParameterPositions(lpar, NULL, rpar, 0);
    } else {
        handleParameterPositions(lpar, commas->next, rpar, 1);
    }
}
