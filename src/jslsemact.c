#include "jslsemact.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "parsers.h"
#include "classcaster.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filetable.h"
#include "jsemact.h"
#include "symbol.h"
#include "list.h"
#include "stackmemory.h"

#include "log.h"
#include "recyacc.h"

S_jslStat *s_jsl;



S_jslClassStat *newJslClassStat(IdList *className, Symbol *thisClass, char *thisPackage, S_jslClassStat *next) {
    S_jslClassStat *jslClassStat;

    jslClassStat = stackMemoryAlloc(sizeof(S_jslClassStat));
    jslClassStat->className = className;
    jslClassStat->thisClass = thisClass;
    jslClassStat->thisPackage = thisPackage;
    jslClassStat->annonInnerCounter = 0;
    jslClassStat->functionInnerCounter = 0;
    jslClassStat->next = next;

    return jslClassStat;
}

void fillJslStat(S_jslStat *jslStat, int pass, int sourceFileNumber, int language, JslTypeTab *typeTab,
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
    jslSymbolList->position = pos;
    jslSymbolList->isExplicitlyImported = isExplicitlyImported;
    jslSymbolList->next = NULL;
}

static void jslCreateTypeSymbolInList(JslSymbolList *ss, char *name) {
    Symbol *s;

    s = newSymbol(name, name, noPosition);
    s->type = TypeStruct;
    s->storage = StorageNone;
    fillJslSymbolList(ss, s, noPosition, false);
}

/* Used as argument to addToFrame() thus void* argument */
static void jslRemoveNestedClass(void *ddv) {
    JslSymbolList *dd;
    bool deleted;

    dd = (JslSymbolList *) ddv;
    //&fprintf(dumpOut, "removing class %s from jsltab\n", dd->d->name);
    log_debug("removing class %s from jsltab", dd->d->name);
    assert(s_jsl!=NULL);
    deleted = jslTypeTabDeleteExact(s_jsl->typeTab, dd);
    assert(deleted);
}

Symbol *jslTypeSymbolDefinition(char *ttt2, IdList *packid,
                                AddYesNo add, int order, bool isExplicitlyImported) {
    char fqtName[MAX_FILE_NAME_SIZE];
    IdList dd2;
    int index;
    Symbol *smemb;
    JslSymbolList ss, *xss, *memb;
    Position *importPos;
    bool isMember; UNUSED isMember; /* Because log_trace() might not generate any code... */

    jslCreateTypeSymbolInList(&ss, ttt2);
    fillfIdList(&dd2, ttt2, NULL, noPosition, ttt2, TypeStruct, packid);
    javaCreateComposedName(NULL,&dd2,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    smemb = javaFQTypeSymbolDefinition(ttt2, fqtName);
    //&fprintf(communicationChannel, "[jsl] jslTypeSymbolDefinition %s, %s, %s, %s\n", ttt2, fqtName, smemb->name, smemb->linkName);
    if (add == ADD_YES) {
        if (packid!=NULL) importPos = &packid->id.position;
        else importPos = &noPosition;
        xss = stackMemoryAlloc(sizeof(JslSymbolList)); // Or in MEMORY_CF ???
        fillJslSymbolList(xss, smemb, *importPos, isExplicitlyImported);
        /* TODO: Why are we using isMember() and not looking at the result? Side-effect? */
        isMember = jslTypeTabIsMember(s_jsl->typeTab, xss, &index, &memb);
        log_trace("[jsl] jslTypeTabIsMember() returned %s", isMember?"true":"false");
        if (order == ORDER_PREPEND) {
            log_debug("[jsl] prepending class %s to jsltab", smemb->name);
            jslTypeTabPush(s_jsl->typeTab, xss, index);
            addToFrame(jslRemoveNestedClass, xss);
        } else {
            log_debug("[jsl] appending class %s to jsltab", smemb->name);
            jslTypeTabSetLast(s_jsl->typeTab, xss, index);
            addToFrame(jslRemoveNestedClass, xss);
        }
    }
    return(smemb);
}

static Symbol *jslTypeSymbolUsage(char *ttt2, IdList *packid) {
    char fqtName[MAX_FILE_NAME_SIZE];
    IdList dd2;
    Symbol *smemb;
    JslSymbolList ss, *memb;

    jslCreateTypeSymbolInList(&ss, ttt2);
    if (packid==NULL && jslTypeTabIsMember(s_jsl->typeTab, &ss, NULL, &memb)) {
        smemb = memb->d;
        return(smemb);
    }
    fillfIdList(&dd2, ttt2, NULL, noPosition, ttt2, TypeStruct, packid);
    javaCreateComposedName(NULL,&dd2,'/',NULL,fqtName,MAX_FILE_NAME_SIZE);
    smemb = javaFQTypeSymbolDefinition(ttt2, fqtName);
    return(smemb);
}

Symbol *jslTypeNameDefinition(IdList *tname) {
    Symbol *memb;
    Symbol *dd;
    TypeModifier *td;

    memb = jslTypeSymbolUsage(tname->id.name, tname->next);
    td = cfAlloc(TypeModifier); //XX_ALLOC?
    initTypeModifierAsStructUnionOrEnum(td, TypeStruct, memb, NULL, NULL);

    dd = cfAlloc(Symbol); //XX_ALLOC?
    fillSymbolWithTypeModifier(dd, memb->name, memb->linkName, tname->id.position, td);

    return dd;
}

Symbol *jslMethodHeader(unsigned modif, Symbol *type,
                          Symbol *decl, int storage, SymbolList *throws) {
    int newFun;

    completeDeclarator(type,decl);
    decl->access = modif;
    assert(s_jsl && s_jsl->classStat && s_jsl->classStat->thisClass);
    if (s_jsl->classStat->thisClass->access & AccessInterface) {
        // set interface default access flags
        decl->access |= (AccessPublic | AccessAbstract);
    }
    decl->storage = storage;
    //& if (modif & AccessStatic) decl->storage = StorageStaticMethod;
    newFun = javaSetFunctionLinkName(s_jsl->classStat->thisClass, decl, MEMORY_CF);
    if (decl->pos.file != olOriginalFileNumber && options.serverOperation == OLO_PUSH) {
        // pre load of saved file akes problem on move field/method, ...
        addMethodCxReferences(modif, decl, s_jsl->classStat->thisClass);
    }
    if (newFun) {
        log_debug("[jsl] adding method %s==%s to %s (at %lx)", decl->name,
                  decl->linkName, s_jsl->classStat->thisClass->linkName, (unsigned long)decl);
        LIST_APPEND(Symbol, s_jsl->classStat->thisClass->u.structSpec->records, decl);
    }
    decl->u.typeModifier->u.m.signature = strchr(decl->linkName, '(');
    decl->u.typeModifier->u.m.exceptions = throws;
    return(decl);
}

#define MAP_FUN_SIGNATURE char *file, char *a1, char *a2, Completions *a3, void *a4, int *a5

void jslAddSuperClassOrInterface(Symbol *memb,Symbol *supp){
    int origin;

    log_debug("loading super/interf %s of %s", supp->linkName, memb->linkName);
    javaLoadClassSymbolsFromFile(supp);
    origin = memb->u.structSpec->classFileNumber;
    addSuperClassOrInterface( memb, supp, origin);
}


void jslAddSuperClassOrInterfaceByName(Symbol *memb,char *super){
    Symbol        *supp;
    supp = javaGetFieldClass(super,NULL);
    jslAddSuperClassOrInterface( memb, supp);
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
    if (recAccessFlags & AccessPublic) {
        log_trace("ret 1 access public");
        return 1;
    }
    if (recAccessFlags & AccessProtected) {
        if (javaClassIsInCurrentPackage(cl)) {
            log_trace("ret 1 protected in current package");
            return 1;
        }
        for (cs=s_jsl->classStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
            if (cs->thisClass == cl) {
                log_trace("ret 1 as it is inside class");
                return 1;
            }
            if (cctIsMember(&cs->thisClass->u.structSpec->casts, cl, 1)) {
                log_trace("ret 1 as it is inside subclass");
                return 1;
            }
        }
        log_trace("ret 0 on protected");
        return 0;
    }
    if (recAccessFlags & AccessPrivate) {
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

    assert(str && str->type==TypeStruct);
    ss = str->u.structSpec;
    assert(ss);
    log_debug("appending %d nested classes of %s", ss->nestedCount, str->linkName);
    for(i=0; i<ss->nestedCount; i++) {
        log_trace("checking %s %s %d %d", ss->nestedClasses[i].cl->name, ss->nestedClasses[i].cl->linkName,ss->nestedClasses[i].membFlag, jslRecordAccessible(str, ss->nestedClasses[i].cl, ss->nestedClasses[i].accFlags));
        if (ss->nestedClasses[i].membFlag && jslRecordAccessible(str, ss->nestedClasses[i].cl, ss->nestedClasses[i].accFlags)) {
            fillId(&ocid, str->linkName, NULL, noPosition);
            fillIdList(&oclassid, ocid, str->linkName, TypeStruct, NULL);
            log_trace("adding %s %s", ss->nestedClasses[i].cl->name, ss->nestedClasses[i].cl->linkName);
            jslTypeSymbolDefinition(ss->nestedClasses[i].cl->name, &oclassid,
                                    ADD_YES, order, false);
        }
    }
}
