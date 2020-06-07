#include "jslsemact.h"

#include "misc.h"
#include "commons.h"
#include "globals.h"
#include "parsers.h"
#include "classcaster.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "jsemact.h"
#include "symbol.h"
#include "list.h"

#include "log.h"
#include "utils.h"
#include "recyacc.h"

S_jslStat *s_jsl;



S_jslClassStat *newJslClassStat(IdList *className, Symbol *thisClass, char *thisPackage, S_jslClassStat *next) {
    S_jslClassStat *jslClassStat;

    XX_ALLOC(jslClassStat, S_jslClassStat);
    jslClassStat->className = className;
    jslClassStat->thisClass = thisClass;
    jslClassStat->thisPackage = thisPackage;
    jslClassStat->annonInnerCounter = 0;
    jslClassStat->functionInnerCounter = 0;
    jslClassStat->next = next;

    return jslClassStat;
}

void fillJslStat(S_jslStat *jslStat, int pass, int sourceFileNumber, int language, S_jslTypeTab *typeTab,
                 S_jslClassStat *classStat, SymbolList *waitList, void /*YYSTYPE*/ *savedyylval,
                 void /*S_yyGlobalState*/ *savedYYstate, int yyStateSize, S_jslStat *next) {
    jslStat->pass = pass;
    jslStat->sourceFileNumber = sourceFileNumber;
    jslStat->language = language;
    jslStat->typeTab = typeTab;
    jslStat->classStat = classStat;
    jslStat->waitList = waitList;
    jslStat->savedyylval = savedyylval;
    jslStat->savedYYstate = savedYYstate;
    jslStat->yyStateSize = yyStateSize;
    jslStat->next = next;
}

static void fillJslSymbolList(JslSymbolList *jslSymbolList, struct symbol *d,
                              struct position pos, bool isExplicitlyImported) {
    jslSymbolList->d = d;
    jslSymbolList->pos = pos;
    jslSymbolList->isExplicitlyImported = isExplicitlyImported;
    jslSymbolList->next = NULL;
}

static void jslCreateTypeSymbolInList(JslSymbolList *ss, char *name) {
    Symbol *s;

    s = newSymbol(name, name, s_noPos);
    fillSymbolBits(&s->bits, ACCESS_DEFAULT, TypeStruct, StorageNone);
    fillJslSymbolList(ss, s, s_noPos, false);
}

Symbol *jslTypeSpecifier2(TypeModifier *t) {
   Symbol *symbol;

    CF_ALLOC(symbol, Symbol);   /* Not in same memory as newSymbol() uses, why? */
    fillSymbolWithType(symbol, NULL, NULL, s_noPos, t);

    return symbol;
}

static TypeModifier *jslCreateSimpleTypeModifier(Type type) {
    TypeModifier *p;

    assert(type>=0 && type<MAX_TYPE);
    if (s_preCreatedTypesTable[type] == NULL) {
        CF_ALLOC(p, TypeModifier);
        initTypeModifier(p, type);
    } else {
        p = s_preCreatedTypesTable[type];
    }
    assert(p->kind == type);

    return p;
}

Symbol *jslTypeSpecifier1(Type t) {
    return jslTypeSpecifier2(jslCreateSimpleTypeModifier(t));
}

TypeModifier *jslAppendComposedType(TypeModifier **d, Type type) {
    TypeModifier *p;
    CF_ALLOC(p, TypeModifier);
    initTypeModifier(p, type);
    LIST_APPEND(TypeModifier, (*d), p);
    return(p);
}

TypeModifier *jslPrependComposedType(TypeModifier *d, Type type) {
    TypeModifier *p;
    CF_ALLOC(p, TypeModifier);
    initTypeModifier(p, type);
    p->next = d;
    return(p);
}

void jslCompleteDeclarator(Symbol *t, Symbol *d) {
    assert(t && d);
    if (t == &s_errorSymbol || d == &s_errorSymbol
        || t->bits.symType==TypeError || d->bits.symType==TypeError) return;
    LIST_APPEND(TypeModifier, d->u.type, t->u.type);
    d->bits.storage = t->bits.storage;
}

static void jslRemoveNestedClass(void  *ddv) {
    JslSymbolList *dd;
    int             check;
    dd = (JslSymbolList *) ddv;
    //&fprintf(dumpOut, "removing class %s from jsltab\n", dd->d->name);
    log_debug("removing class %s from jsltab", dd->d->name);
    assert(s_jsl!=NULL);
    check = jslTypeTabDeleteExact(s_jsl->typeTab, dd);
    assert(check);
}

Symbol *jslTypeSymbolDefinition(char *ttt2, IdList *packid,
                                AddYesNo add, int order, bool isExplicitlyImported) {
    char fqtName[MAX_FILE_NAME_SIZE];
    IdList dd2;
    int ii;
    Symbol *smemb;
    JslSymbolList ss, *xss, *memb;
    Position *importPos;
    int mm; UNUSED mm;

    jslCreateTypeSymbolInList(&ss, ttt2);
    fillfIdList(&dd2, ttt2, NULL, s_noPos, ttt2, TypeStruct, packid);
    javaCreateComposedName(NULL,&dd2,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    smemb = javaFQTypeSymbolDefinition(ttt2, fqtName);
    //&fprintf(ccOut, "[jsl] jslTypeSymbolDefinition %s, %s, %s, %s\n", ttt2, fqtName, smemb->name, smemb->linkName);
    if (add == ADD_YES) {
        if (packid!=NULL) importPos = &packid->id.p;
        else importPos = &s_noPos;
        XX_ALLOC(xss, JslSymbolList); // CF_ALLOC ???
        fillJslSymbolList(xss, smemb, *importPos, isExplicitlyImported);
        /* TODO: Why are we using isMember() and not looking at the result? Side-effect? */
        mm = jslTypeTabIsMember(s_jsl->typeTab, xss, &ii, &memb);
        log_trace("[jsl] jslTypeTabIsMember() returned %s", mm?"true":"false");
        if (order == ORDER_PREPEND) {
            log_debug("[jsl] prepending class %s to jsltab", smemb->name);
            jslTypeTabSet(s_jsl->typeTab, xss, ii);
            addToTrail(jslRemoveNestedClass, xss);
        } else {
            log_debug("[jsl] appending class %s to jsltab", smemb->name);
            jslTypeTabSetLast(s_jsl->typeTab, xss, ii);
            addToTrail(jslRemoveNestedClass, xss);
        }
    }
    return(smemb);
}

static Symbol *jslTypeSymbolUsage(char *ttt2, IdList *packid) {
    char fqtName[MAX_FILE_NAME_SIZE];
    IdList dd2;
    int ii;
    Symbol *smemb;
    JslSymbolList ss, *memb;

    jslCreateTypeSymbolInList(&ss, ttt2);
    if (packid==NULL && jslTypeTabIsMember(s_jsl->typeTab, &ss, &ii, &memb)) {
        smemb = memb->d;
        return(smemb);
    }
    fillfIdList(&dd2, ttt2, NULL, s_noPos, ttt2, TypeStruct, packid);
    javaCreateComposedName(NULL,&dd2,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    smemb = javaFQTypeSymbolDefinition(ttt2, fqtName);
    return(smemb);
}

Symbol *jslTypeNameDefinition(IdList *tname) {
    Symbol *memb;
    Symbol *dd;
    TypeModifier *td;

    memb = jslTypeSymbolUsage(tname->id.name, tname->next);
    CF_ALLOC(td, TypeModifier); //XX_ALLOC?
    initTypeModifierAsStructUnionOrEnum(td, TypeStruct, memb, NULL, NULL);

    CF_ALLOC(dd, Symbol); //XX_ALLOC?
    fillSymbolWithType(dd, memb->name, memb->linkName, tname->id.p, td);

    return dd;
}

static int jslClassifySingleAmbigNameToTypeOrPack(IdList *name,
                                                  Symbol **str
                                                  ){
    JslSymbolList ss, *memb, *nextmemb;
    int ii, haveit;

    jslCreateTypeSymbolInList(&ss, name->id.name);
    log_trace("looking for '%s'", name->id.name);
    if (jslTypeTabIsMember(s_jsl->typeTab, &ss, &ii, &memb)) {
        /* a type */
        log_trace("found '%s'", memb->d->linkName);
        assert(memb);
        // O.K. I have to load the class in order to check its access flags
        for(; memb!=NULL; memb=nextmemb) {
            nextmemb = memb;
            jslTypeTabNextMember(&ss, &nextmemb);
            haveit = 1;
            if (nextmemb!=NULL) {
                // load the class only if there is an ambiguity,
                // so it does not slow down indexing
                javaLoadClassSymbolsFromFile(memb->d);
                haveit = javaOuterClassAccessible(memb->d);
            }
            if (haveit) {
                log_trace("O.K. class '%s' is accessible", memb->d->name);
                *str = memb->d;
                name->nameType = TypeStruct;
                name->fname = memb->d->linkName;
                return(TypeStruct);
            }
        }
    }
    name->nameType = TypePackage;
    return(TypePackage);
}

int jslClassifyAmbiguousTypeName(IdList *name, Symbol **str) {
    int         pres;
    Symbol    *pstr;
    assert(name);
    *str = &s_errorSymbol;
    if (name->next == NULL) {
        /* a single name */
        jslClassifySingleAmbigNameToTypeOrPack( name, str);
    } else {
        /* composed name */
        pres = jslClassifyAmbiguousTypeName(name->next, &pstr);
        switch (pres) {
        case TypePackage:
            if (javaTypeFileExist(name)) {
                name->nameType = TypeStruct;
                *str = jslTypeSymbolUsage(name->id.name, name->next);
            } else {
                name->nameType = TypePackage;
            }
            break;
        case TypeStruct:
            name->nameType = TypeStruct;
            *str = jslTypeSymbolUsage(name->id.name, name->next);
            break;
        default: assert(0);
        }
    }
    return(name->nameType);
}

Symbol *jslPrependDirectEnclosingInstanceArgument(Symbol *args) {
    warningMessage(ERR_ST,"[jslPrependDirectEnclosingInstanceArgument] not yet implemented");
    return(args);
}

Symbol *jslMethodHeader(unsigned modif, Symbol *type,
                          Symbol *decl, int storage, SymbolList *throws) {
    int newFun;

    completeDeclarator(type,decl);
    decl->bits.access = modif;
    assert(s_jsl && s_jsl->classStat && s_jsl->classStat->thisClass);
    if (s_jsl->classStat->thisClass->bits.access & ACCESS_INTERFACE) {
        // set interface default access flags
        decl->bits.access |= (ACCESS_PUBLIC | ACCESS_ABSTRACT);
    }
    decl->bits.storage = storage;
    //& if (modif & ACCESS_STATIC) decl->bits.storage = StorageStaticMethod;
    newFun = javaSetFunctionLinkName(s_jsl->classStat->thisClass, decl, MEMORY_CF);
    if (decl->pos.file != s_olOriginalFileNumber && s_opt.server_operation == OLO_PUSH) {
        // pre load of saved file akes problem on move field/method, ...
        addMethodCxReferences(modif, decl, s_jsl->classStat->thisClass);
    }
    if (newFun) {
        log_debug("[jsl] adding method %s==%s to %s (at %lx)", decl->name,
                  decl->linkName, s_jsl->classStat->thisClass->linkName, (unsigned long)decl);
        LIST_APPEND(Symbol, s_jsl->classStat->thisClass->u.s->records, decl);
    }
    decl->u.type->u.m.signature = strchr(decl->linkName, '(');
    decl->u.type->u.m.exceptions = throws;
    return(decl);
}

void jslAddMapedImportTypeName(
                               char *file,
                               char *path,
                               char *pack,
                               Completions *c,
                               void *vdirid,
                               int  *storage
                               ) {
    char *p;
    char ttt2[MAX_FILE_NAME_SIZE];
    int len2;
    IdList *packid;

    //&fprintf(ccOut,":jsl import type %s %s %s\n", file, path, pack);
    packid = (IdList *) vdirid;
    for(p=file; *p && *p!='.' && *p!='$'; p++) ;
    if (*p != '.') return;
    if (strcmp(p,".class")!=0 && strcmp(p,".java")!=0) return;
    len2 = p - file;
    strncpy(ttt2, file, len2);
    assert(len2+1 < MAX_FILE_NAME_SIZE);
    ttt2[len2] = 0;
    jslTypeSymbolDefinition(ttt2, packid, ADD_YES, ORDER_APPEND, false);
}

void jslAddAllPackageClassesFromFileTab(IdList *packid) {
    int i;
    FileItem *ff;
    int pnlen, c;
    char fqtName[MAX_FILE_NAME_SIZE];
    char ttt[MAX_FILE_NAME_SIZE];
    char *ee, *bb, *dd;

    javaCreateComposedName(NULL,packid,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    pnlen = strlen(fqtName);
    for(i=0; i<s_fileTab.size; i++) {
        ff = s_fileTab.tab[i];
        if (ff!=NULL
            && ff->name[0]==ZIP_SEPARATOR_CHAR
            && strncmp(ff->name+1, fqtName, pnlen)==0
            && (packid==NULL || ff->name[pnlen+1] == '/')) {
            if (packid==NULL) bb = ff->name+pnlen+1;
            else bb = ff->name+pnlen+2;
            c = 0;
            for(ee=bb, dd=ttt; *ee; ee++,dd++) {
                c = *ee;
                if (c=='.' || c=='/' || c=='$') {
                    *dd = 0;
                    break;
                } else {
                    *dd = c;
                }
            }
            if (c=='.') {
                jslTypeSymbolDefinition(ttt, packid, ADD_YES, ORDER_APPEND, false);
            }
        }
    }
}

static void jslAddToLoadWaitList( Symbol *clas ) {
    SymbolList *ll;

    CF_ALLOC(ll, SymbolList);
    /* REPLACED: FILL_symbolList(ll, clas, s_jsl->waitList); with compound literal */
    *ll = (SymbolList){.d = clas, .next = s_jsl->waitList};
    s_jsl->waitList = ll;
}


void jslAddSuperClassOrInterface(Symbol *memb,Symbol *supp){
    int origin;

    log_debug("loading super/interf %s of %s", supp->linkName, memb->linkName);
    javaLoadClassSymbolsFromFile(supp);
    origin = memb->u.s->classFile;
    addSuperClassOrInterface( memb, supp, origin);
}


void jslAddSuperClassOrInterfaceByName(Symbol *memb,char *super){
    Symbol        *supp;
    supp = javaGetFieldClass(super,NULL);
    jslAddSuperClassOrInterface( memb, supp);
}

static void jslAddNestedClass(Symbol *inner, Symbol *outer, int memb,
                              int accessFlags) {
    int n;

    assert(outer && outer->bits.symType==TypeStruct && outer->u.s);
    n = outer->u.s->nestedCount;
    log_debug("adding nested %s of %s(at %lx)[%d] --> %s to %s", inner->name, outer->name, (unsigned long)outer, n, inner->linkName, outer->linkName);
    if (n == 0) {
        CF_ALLOCC(outer->u.s->nest, MAX_INNERS_CLASSES, S_nestedSpec);
    }
    // avoid multiple occurences, rather not, as it must correspond to
    // file processing order
    //& for(i=0; i<n; i++) if (outer->u.s->nest[i].cl == inner) return;
    fill_nestedSpec(&(outer->u.s->nest[n]), inner, memb, accessFlags);
    outer->u.s->nestedCount ++;
    if (outer->u.s->nestedCount >= MAX_INNERS_CLASSES) {
        fatalError(ERR_ST,"number of nested classes overflowed over MAX_INNERS_CLASSES", XREF_EXIT_ERR);
    }
}

// BERK, there is a copy of this function in semact.c (javaRecordAccessible)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// when modifying this, you will need to change it there too
// TODO: So, why don't we extract this common functionality?!?!?!?
static int jslRecordAccessible(Symbol *cl, Symbol *rec, unsigned recAccessFlags) {
    S_jslClassStat *cs, *lcs;
    int len;

    if (cl == NULL)
        return 1;  /* argument or local variable */

    log_trace("testing accessibility %s . %s of 0%o",cl->linkName, rec->linkName, recAccessFlags);
    assert(s_jsl && s_jsl->classStat);
    if (recAccessFlags & ACCESS_PUBLIC) {
        log_trace("ret 1 access public");
        return 1;
    }
    if (recAccessFlags & ACCESS_PROTECTED) {
        if (javaClassIsInCurrentPackage(cl)) {
            log_trace("ret 1 protected in current package");
            return 1;
        }
        for (cs=s_jsl->classStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            if (cs->thisClass == cl) {
                log_trace("ret 1 as it is inside class");
                return 1;
            }
            if (cctIsMember(&cs->thisClass->u.s->casts, cl, 1)) {
                log_trace("ret 1 as it is inside subclass");
                return 1;
            }
        }
        log_trace("ret 0 on protected");
        return 0;
    }
    if (recAccessFlags & ACCESS_PRIVATE) {
        for(lcs=cs=s_jsl->classStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            lcs = cs;
        }
        if (lcs!=NULL && lcs->thisClass!=NULL) {
            log_trace("comparing %s and %s", lcs->thisClass->linkName, cl->linkName);
            len = strlen(lcs->thisClass->linkName);
            if (strncmp(lcs->thisClass->linkName, cl->linkName, len)==0) {
                log_trace("ret 1 private inside the class");
                return 1;
            }
        }
        log_trace("ret 0 on private");
        return 0;
    }
    /* default access */
    if (javaClassIsInCurrentPackage(cl)) {
        log_trace("ret 1 default protection in current package");
        return 1;
    }
    log_trace("ret 0 on default");
    return 0;
}


void jslAddNestedClassesToJslTypeTab( Symbol *str, int order) {
    S_symStructSpec *ss;
    Id ocid;
    IdList oclassid;
    int i;

    assert(str && str->bits.symType==TypeStruct);
    ss = str->u.s;
    assert(ss);
    log_debug("appending %d nested classes of %s", ss->nestedCount, str->linkName);
    for(i=0; i<ss->nestedCount; i++) {
        log_trace("checking %s %s %d %d", ss->nest[i].cl->name, ss->nest[i].cl->linkName,ss->nest[i].membFlag, jslRecordAccessible(str, ss->nest[i].cl, ss->nest[i].accFlags));
        if (ss->nest[i].membFlag && jslRecordAccessible(str, ss->nest[i].cl, ss->nest[i].accFlags)) {
            fillId(&ocid, str->linkName, NULL, s_noPos);
            fillIdList(&oclassid, ocid, str->linkName, TypeStruct, NULL);
            log_trace("adding %s %s", ss->nest[i].cl->name, ss->nest[i].cl->linkName);
            jslTypeSymbolDefinition(ss->nest[i].cl->name, &oclassid,
                                    ADD_YES, order, false);
        }
    }
}


void jslAddSuperNestedClassesToJslTypeTab( Symbol *cc) {
    SymbolList *ss;
    for(ss=cc->u.s->super; ss!=NULL; ss=ss->next) {
        jslAddSuperNestedClassesToJslTypeTab(ss->d);
    }
    jslAddNestedClassesToJslTypeTab(cc, ORDER_PREPEND);
}


void jslNewClassDefinitionBegin(Id *name,
                                int accFlags,
                                Symbol *anonInterf,
                                int position
                                ) {
    char                ttt[TMP_STRING_SIZE];
    char                tttn[TMP_STRING_SIZE];
    S_jslClassStat      *nss;
    Id           *inname;
    IdList       *ill, mntmp;
    Symbol            *cc;
    int                 fileInd, membflag, cn;

    inname = name;
    if (position==CPOS_FUNCTION_INNER || anonInterf!=NULL) {
        if (position==CPOS_FUNCTION_INNER) {
            /* inner class defined inside a method */
            s_jsl->classStat->functionInnerCounter++;
            sprintf(tttn, "%d", s_jsl->classStat->functionInnerCounter);
            sprintf(ttt, "%s", inname->name);
            fillfIdList(&mntmp, tttn, NULL, s_noPos, tttn, TypeStruct, s_jsl->classStat->className);
            // this is a very special reason why to do TYPE_ADD_YES here,
            // because method nested class will not be added as class nested
            // at the end of this function
            cc = jslTypeSymbolDefinition(ttt, &mntmp,
                                         ADD_YES, ORDER_PREPEND, false);
        } else {
            /* anonymous class implementing an interface */
            s_jsl->classStat->annonInnerCounter++;
            sprintf(ttt, "%d$%s", s_jsl->classStat->annonInnerCounter,
                    inname->name);
            cc = jslTypeSymbolDefinition(ttt,s_jsl->classStat->className,
                                         ADD_NO,ORDER_PREPEND, false);
        }
    } else {
        sprintf(ttt, "%s", inname->name);
        cc = jslTypeSymbolDefinition(ttt,s_jsl->classStat->className,
                                     ADD_NO,ORDER_PREPEND, false);
    }
    cc->bits.access = accFlags;
    log_trace("reading class %s [%x] at %x", cc->linkName, cc->bits.access, cc);
    if (s_jsl->classStat->next != NULL) {
        /* nested class, add it to its outer class list */
        if (s_jsl->classStat->thisClass->bits.access & ACCESS_INTERFACE) {
            accFlags |= (ACCESS_PUBLIC | ACCESS_STATIC);
            cc->bits.access = accFlags;
        }
        membflag = (anonInterf==NULL && position!=CPOS_FUNCTION_INNER);
        if (s_jsl->pass==1) {
            jslAddNestedClass(cc, s_jsl->classStat->thisClass, membflag, accFlags);
            cn = cc->u.s->classFile;
            assert(s_fileTab.tab[cn]);
            if (! (accFlags & ACCESS_STATIC)) {
                // note that non-static direct enclosing class exists
                // I am putting in comment just by prudence, but you can
                // freely uncoment it
                assert(s_jsl->classStat->thisClass && s_jsl->classStat->thisClass->u.s);
                assert(s_jsl->classStat->thisClass->bits.symType==TypeStruct);
                s_fileTab.tab[cn]->directEnclosingInstance = s_jsl->classStat->thisClass->u.s->classFile;
                log_trace("setting dei %d->%d of %s, none==%d", cn,  s_jsl->classStat->thisClass->u.s->classFile,
                          s_fileTab.tab[cn]->name, s_noneFileIndex);
            } else {
                s_fileTab.tab[cn]->directEnclosingInstance = s_noneFileIndex;
            }
        }
    }

    // add main class name
    if (s_jsl->classStat->next==NULL && s_jsl->pass==1) {
        /* top level class */
        jslTypeSymbolDefinition(cc->name,s_jsl->classStat->className,
                                ADD_YES, ORDER_PREPEND, false);
    }

    assert(cc && cc->u.s && s_fileTab.tab[cc->u.s->classFile]);
    assert(s_jsl->sourceFileNumber>=0 && s_jsl->sourceFileNumber!=s_noneFileIndex);
    assert(s_fileTab.tab[s_jsl->sourceFileNumber]);
    fileInd = cc->u.s->classFile;
    log_trace("setting source file of %s to %s", s_fileTab.tab[cc->u.s->classFile]->name,
              s_fileTab.tab[s_jsl->sourceFileNumber]->name);
    s_fileTab.tab[fileInd]->b.sourceFileNumber = s_jsl->sourceFileNumber;

    if (accFlags & ACCESS_INTERFACE) s_fileTab.tab[fileInd]->b.isInterface = true;
    addClassTreeHierarchyReference(fileInd,&inname->p,UsageClassTreeDefinition);
    if (inname->p.file != s_olOriginalFileNumber && s_opt.server_operation == OLO_PUSH) {
        // pre load of saved file akes problem on move field/method, ...
        addCxReference(cc, &inname->p, UsageDefined,s_noneFileIndex, s_noneFileIndex);
    }
    // this is to update references affected to class file before
    // if you remove this, then remove also at class end
    // berk, this removes all usages to be loaded !!
    //& s_fileTab.tab[fileInd]->b.cxLoading = true;
    // here reset the innerclasses number, so the next call will
    // surely allocate the table and will start from the first one
    // it is a little bit HACKED :)
    if (s_jsl->pass==1)
        cc->u.s->nestedCount = 0;

    stackMemoryBlockStart();
    XX_ALLOC(ill, IdList);
    fillfIdList(ill, cc->name, inname->symbol, inname->p, cc->name, TypeStruct, s_jsl->classStat->className);
    nss = newJslClassStat(ill, cc, s_jsl->classStat->thisPackage,
                          s_jsl->classStat);
    s_jsl->classStat = nss;
    javaCreateClassFileItem(cc);
    cc->bits.javaFileIsLoaded = 1;
    cc->bits.javaSourceIsLoaded = 1;
    if (anonInterf!=NULL && s_jsl->pass==2) {
        /* anonymous implementing an interface, one more time */
        /* now put there object as superclass and its interface */
        // it was originally in reverse order, changed at 13/1/2001
        if (anonInterf->bits.access&ACCESS_INTERFACE) {
            jslAddSuperClassOrInterfaceByName(cc, s_javaLangObjectLinkName);
        }
        jslAddSuperClassOrInterfaceByName(cc, anonInterf->linkName);
    }
    if (s_jsl->pass==2) {
        jslAddToLoadWaitList(cc);
        jslAddSuperNestedClassesToJslTypeTab(cc);
    }
}

void jslNewClassDefinitionEnd(void) {
    Symbol *cc;
    int fileInd;
    assert(s_jsl->classStat && s_jsl->classStat->next);

    cc = s_jsl->classStat->thisClass;
    fileInd = cc->u.s->classFile;
    if (s_fileTab.tab[fileInd]->b.cxLoading) {
        s_fileTab.tab[fileInd]->b.cxLoaded = true;
    }

    s_jsl->classStat = s_jsl->classStat->next;
    stackMemoryBlockFree();
}

void jslAddDefaultConstructor(Symbol *cl) {
    Symbol *cc;
    cc = javaCreateNewMethod(cl->name, &s_noPos, MEMORY_CF);
    jslMethodHeader(cl->bits.access, &s_defaultVoidDefinition, cc,
                    StorageConstructor, NULL);
}

void jslNewAnonClassDefinitionBegin(Id *interfName) {
    IdList   ll;
    Symbol        *interf,*str;
    //& XX_ALLOC(ll, IdList);
    fillIdList(&ll, *interfName, interfName->name, TypeDefault, NULL);
    jslClassifyAmbiguousTypeName(&ll, &str);
    interf = jslTypeNameDefinition(&ll);
    jslNewClassDefinitionBegin(&s_javaAnonymousClassName, ACCESS_DEFAULT,
                               interf, CPOS_ST);
}
