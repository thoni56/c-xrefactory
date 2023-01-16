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


void fillRecFindStr(S_recFindStr *recFindStr, Symbol *baseClass, Symbol *currentClass, Symbol *nextRecord,
                    unsigned recsClassCounter) {
    recFindStr->baseClass            = baseClass;
    recFindStr->currentClass         = currentClass;
    recFindStr->nextRecord           = nextRecord;
    recFindStr->recsClassCounter     = recsClassCounter;
    recFindStr->superClassesCount    = 0;
    recFindStr->anonymousUnionsCount = 0;
}

bool displayingErrorMessages(void) {
    // no error messages for Java file preloaded for symbols
    if (LANGUAGE(LANG_JAVA) && s_jsl!=NULL)
        return false;
    if (options.debug || options.errors)
        return true;
    return false;
}

int styyerror(char *message) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (strcmp(message, "syntax error") == 0) {
        if (displayingErrorMessages()) {
            log_trace("Syntax error on: '%s'", yytext);
            sprintf(tmpBuff, "Syntax error on: %s", yytext);
            errorMessage(ERR_ST, tmpBuff);
        }
    } else if (strncmp(message, "DEBUG:", 6) == 0) {
        if (displayingErrorMessages())
            errorMessage(ERR_ST, message);
    } else {
        sprintf(tmpBuff,"YACC error: %s", message);
        errorMessage(ERR_INTERNAL, tmpBuff);
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


/* Used as action in addToTrail(), thus void* */
void setToNull(void *p) {
    void **pp;
    pp = (void **)p;
    *pp = NULL;
}


/* Used as action in addToTrail(), thus void* */
void deleteSymDef(void *p) {
    Symbol        *pp;

    pp = (Symbol *) p;
    log_debug("deleting %s %s", pp->name, pp->linkName);
    if (symbolTableDelete(javaStat->locals,pp)) return;
    if (symbolTableDelete(symbolTable,pp)==0) {
        assert(options.mode);
        if (options.mode != ServerMode) {
            errorMessage(ERR_INTERNAL,"symbol on deletion not found");
        }
    }
}

void unpackPointers(Symbol *symbol) {
    for (int i=0; i<symbol->npointers; i++) {
        appendComposedType(&symbol->u.typeModifier, TypePointer);
    }
    symbol->npointers=0;
}

void addSymbol(SymbolTable *table, Symbol *symbol) {
    /*  a bug can produce, if you add a symbol into old table, and the same
        symbol exists in a newer one. Then it will be deleted from the newer
        one. All this story is about storing information in trail. It should
        containt, both table and pointer !!!!
    */
    log_debug("adding symbol %s: %s %s", symbol->name, typeNamesTable[symbol->type],
              storageEnumName[symbol->storage]);
    assert(symbol->npointers==0);
    addSymbolNoTrail(table, symbol);
    addToTrail(deleteSymDef, symbol /* TODO? Should also include reference to table */, (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)));
}

void recFindPush(Symbol *str, S_recFindStr *rfs) {
    S_symStructSpec *ss;

    assert(str && (str->type==TypeStruct || str->type==TypeUnion));
    if (rfs->recsClassCounter==0) {
        // this is hack to avoid problem when overloading to zero
        rfs->recsClassCounter++;
    }
    ss = str->u.structSpec;
    rfs->nextRecord = ss->records;
    rfs->currentClass                         = str;
    rfs->superClasses[rfs->superClassesCount] = ss->super;
    assert(rfs->superClassesCount < MAX_INHERITANCE_DEEP);
    rfs->superClassesCount++;
}

S_recFindStr *iniFind(Symbol *s, S_recFindStr *rfs) {
    assert(s);
    assert(s->type == TypeStruct || s->type == TypeUnion);
    assert(s->u.structSpec);
    assert(rfs);
    fillRecFindStr(rfs, s, NULL, NULL,s_recFindCl++);
    recFindPush(s, rfs);
    return rfs;
}

void setDirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.mode);
    if (options.mode == ServerMode) {
        s_structRecordCompletionType = typeModifier;
        assert(s_structRecordCompletionType);
    }
}

void setIndirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (typeModifier->kind==TypePointer || typeModifier->kind==TypeArray) {
            s_structRecordCompletionType = typeModifier->next;
            assert(s_structRecordCompletionType);
        } else s_structRecordCompletionType = &errorModifier;
    }
}

bool javaOuterClassAccessible(Symbol *cl) {
    log_trace("testing class accessibility of %s",cl->linkName);
    if (cl->access & AccessPublic) {
        log_trace("return true for public access");
        return true;
    }
    /* default access, check whether it is in current package */
    assert(javaStat);
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
    for (i = 0; i < rfs->superClassesCount - 1; i++) {
        assert(cc);
        for(sups=cc->u.structSpec->super; sups!=NULL; sups=sups->next) {
            if (sups->next == rfs->superClasses[i])
                break;
        }
        if (sups != NULL && sups->next == rfs->superClasses[i]) {
            if (! javaClassIsInCurrentPackage(sups->element))
                return false;
        }
        cc = sups->element;
    }
    assert(cc == rfs->currentClass);
    return true;
}

// BERK, there is a copy of this function in jslsemact.c (jslRecordAccessible)
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// when modifying this, you will need to change it there too
// So, why don't we extract this common functionality?!?!?!?
bool javaRecordAccessible(S_recFindStr *rfs, Symbol *appcl, Symbol *funcl, Symbol *rec, unsigned recAccessFlags) {
    JavaStat          *cs, *lcs;
    int                 len;
    if (funcl == NULL)
        return true;  /* argument or local variable */
    log_trace("testing accessibility %s . %s of x%x",funcl->linkName,rec->linkName, recAccessFlags);
    assert(javaStat);
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
        for (cs=javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
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
        for(lcs=cs=javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
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
    return javaRecordVisible(rfs->baseClass, rfs->currentClass, r->access) &&
           javaRecordAccessible(rfs, rfs->baseClass, rfs->currentClass, r, r->access);
}

int javaGetMinimalAccessibility(S_recFindStr *rfs, Symbol *r) {
    int acc, i;
    for (i=MAX_REQUIRED_ACCESS; i>0; i--) {
        acc = javaRequiredAccessibilityTable[i];
        if (javaRecordVisible(rfs->baseClass, rfs->currentClass, acc) &&
            javaRecordAccessible(rfs, rfs->baseClass, rfs->currentClass, r, acc)) {
            return i;
        }
    }
    return i;
}

Result findStrRecordSym(S_recFindStr *ss, char *recname,            /* can be NULL */
                     Symbol **res, int javaClassif,              /* classify to method/field*/
                     AccessibilityCheckYesNo accessibilityCheck, /* java check accessibility */
                     VisibilityCheckYesNo    visibilityCheck     /* redundant, always equal to accCheck? */
) {
    Symbol     *s, *r, *cclass;
    SymbolList *sss;
    int         m;

    assert(accessibilityCheck == ACCESSIBILITY_CHECK_YES || accessibilityCheck == ACCESSIBILITY_CHECK_NO);
    assert(visibilityCheck == VISIBILITY_CHECK_YES || visibilityCheck == VISIBILITY_CHECK_NO);

    //&fprintf(dumpOut,":\nNEW SEARCH\n"); fflush(dumpOut);
    for (;;) {
        assert(ss);
        cclass = ss->currentClass;
        if (cclass != NULL && cclass->u.structSpec->recSearchCounter == ss->recsClassCounter) {
            // to avoid multiple pass through the same super-class ??
            //&fprintf(dumpOut,":%d==%d --> skipping class
            //%s\n",cclass->u.structSpec->recSearchCounter,ss->recsClassCounter,cclass->linkName);
            goto nextClass;
        }
        if (cclass != NULL)
            log_trace(":looking in class %s(%d)", cclass->linkName, ss->superClassesCount);
        for (r = ss->nextRecord; r != NULL; r = r->next) {
            // special gcc extension of anonymous struct record
            if (r->name != NULL && *r->name == 0 && r->type == TypeDefault &&
                r->u.typeModifier->kind == TypeAnonymousField && r->u.typeModifier->next != NULL &&
                (r->u.typeModifier->next->kind == TypeUnion || r->u.typeModifier->next->kind == TypeStruct)) {
                // put the anonymous union as 'super class'
                if (ss->anonymousUnionsCount + 1 < MAX_ANONYMOUS_FIELDS) {
                    ss->anonymousUnions[ss->anonymousUnionsCount++] = r->u.typeModifier->next->u.t;
                }
            }
            //&fprintf(dumpOut,":checking %s\n",r->name); fflush(dumpOut);
            if (recname == NULL || strcmp(r->name, recname) == 0) {
                if (!LANGUAGE(LANG_JAVA)) {
                    *res           = r;
                    ss->nextRecord = r->next;
                    return RESULT_OK;
                }
                //&fprintf(dumpOut,"acc O.K., checking classif %d\n",javaClassif);fflush(dumpOut);
                if (javaClassif != CLASS_TO_ANY) {
                    assert(r->type == TypeDefault);
                    assert(r->u.typeModifier);
                    m = r->u.typeModifier->kind;
                    if (m == TypeFunction && javaClassif != CLASS_TO_METHOD)
                        goto nextRecord;
                    if (m != TypeFunction && javaClassif == CLASS_TO_METHOD)
                        goto nextRecord;
                }
                //&if(cclass!=NULL)fprintf(dumpOut,"name O.K., checking accesibility %xd
                //%xd\n",cclass->access,r->access); fflush(dumpOut);
                // I have it, check visibility and accessibility
                assert(r);
                if (visibilityCheck == VISIBILITY_CHECK_YES) {
                    if (!javaRecordVisible(ss->baseClass, cclass, r->access)) {
                        // WRONG? return, Doesn't it iverrides any other of this name
                        // Yes, definitely correct, in the first step determining
                        // class to search
                        ss->nextRecord = NULL;
                        *res           = &errorSymbol;
                        return RESULT_NOT_FOUND;
                    }
                }
                if (accessibilityCheck == ACCESSIBILITY_CHECK_YES) {
                    if (!javaRecordAccessible(ss, ss->baseClass, cclass, r, r->access)) {
                        if (visibilityCheck == VISIBILITY_CHECK_YES) {
                            ss->nextRecord = NULL;
                            *res           = &errorSymbol;
                            return RESULT_NOT_FOUND;
                        } else {
                            goto nextRecord;
                        }
                    }
                }
                *res           = r;
                ss->nextRecord = r->next;
                return RESULT_OK;
            }
        nextRecord:;
        }
    nextClass:
        if (ss->anonymousUnionsCount != 0) {
            // O.K. try first to pas to anonymous record
            s = ss->anonymousUnions[--ss->anonymousUnionsCount];
        } else {
            // mark the class as processed
            if (cclass != NULL) {
                cclass->u.structSpec->recSearchCounter = ss->recsClassCounter;
            }

            while (ss->superClassesCount > 0 && ss->superClasses[ss->superClassesCount - 1] == NULL)
                ss->superClassesCount--;
            if (ss->superClassesCount == 0) {
                ss->nextRecord = NULL;
                *res           = &errorSymbol;
                return RESULT_NOT_FOUND;
            }
            sss                 = ss->superClasses[ss->superClassesCount - 1];
            s                   = sss->element;

            ss->superClasses[ss->superClassesCount - 1] = sss->next;
            assert(s && (s->type == TypeStruct || s->type == TypeUnion));
            //& fprintf(dumpOut,":pass to super class %s(%d)\n",s->linkName,ss->superClassesCount);
            //fflush(dumpOut);
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
    int minacc;

    ref = NULL;
    // when in java, then always in qualified name, so access and visibility checks
    // are useless.
    Result rr = findStrRecordSym(iniFind(sym,&rfs),record->name,res,
                          javaClassif, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO);
    if (rr == RESULT_OK && rfs.currentClass != NULL &&
        ((*res)->storage == StorageField || (*res)->storage == StorageMethod ||
         (*res)->storage == StorageConstructor)) {
        assert(rfs.currentClass->u.structSpec && rfs.baseClass && rfs.baseClass->u.structSpec);
        if ((options.ooChecksBits & OOC_ALL_CHECKS) == 0 ||
            javaRecordVisibleAndAccessible(&rfs, rfs.baseClass, rfs.currentClass, *res)) {
            minacc = javaGetMinimalAccessibility(&rfs, *res);
            fillUsage(&usage, UsageUsed, minacc);
            ref = addNewCxReference(*res, &record->position, usage, rfs.currentClass->u.structSpec->classFileNumber,
                                    rfs.baseClass->u.structSpec->classFileNumber);
            // this is adding reference to 'super', not to the field!
            // for pull-up/push-down
            if (super!=NULL) addThisCxReferences(javaStat->classFileNumber,&super->position);
        }
    } else if (rr == RESULT_OK) {
        ref = addCxReference(*res,&record->position,UsageUsed, NO_FILE_NUMBER, NO_FILE_NUMBER);
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
        *resultingSymbol = &errorSymbol;
        goto fini;
    }
    reference = findStrRecordFromSymbol(structure->u.t, field, resultingSymbol, javaClassifier, NULL);
 fini:
    return reference;
}

void labelReference(Id *id, UsageKind usage) {
    char tempString[TMP_STRING_SIZE];
    char *t;
    assert(id);
    if (LANGUAGE(LANG_JAVA)) {
        assert(javaStat&&javaStat->thisClass&&javaStat->thisClass->u.structSpec);
        if (parsedClassInfo.function!=NULL) {
            sprintf(tempString,"%x-%s.%s",javaStat->thisClass->u.structSpec->classFileNumber,
                    parsedClassInfo.function->name, id->name);
        } else {
            sprintf(tempString,"%x-.%s", javaStat->thisClass->u.structSpec->classFileNumber,
                    id->name);
        }
    } else if (parsedClassInfo.function!=NULL) {
        t = strmcpy(tempString, parsedClassInfo.function->name);
        *t = '.';
        t = strcpy(t+1,id->name);
    } else {
        strcpy(tempString, id->name);
    }
    assert(strlen(tempString)<TMP_STRING_SIZE-1);
    addTrivialCxReference(tempString, TypeLabel,StorageDefault, id->position, usage);
}

void setLocalVariableLinkName(struct symbol *p) {
    char ttt[TMP_STRING_SIZE];
    char nnn[TMP_STRING_SIZE];
    int len,tti;
    if (options.serverOperation == OLO_EXTRACT) {
        // extract variable, must pass all needed informations in linkname
        sprintf(nnn, "%c%s%c", LINK_NAME_SEPARATOR, p->name, LINK_NAME_SEPARATOR);
        ttt[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
        // why it commented out ?
        //& if ((!LANGUAGE(LANG_JAVA))
        //&     && (p->u.typeModifier->kind == TypeUnion || p->u.typeModifier->kind == TypeStruct)) {
        //&     ttt[0] = LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
        //& }
        sprintf(ttt+1,"%s", storageNamesTable[p->storage]);
        tti = strlen(ttt);
        len = TMP_STRING_SIZE - tti;
        typeSPrint(ttt+tti, &len, p->u.typeModifier, nnn, LINK_NAME_SEPARATOR, 0,1,SHORT_NAME, NULL);
        sprintf(ttt+tti+len,"%c%x-%x-%x-%x", LINK_NAME_SEPARATOR,
                p->pos.file,p->pos.line,p->pos.col, counters.localVar++);
    } else {
        if (p->storage==StorageExtern && ! options.exactPositionResolve) {
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
        basefname=inputFileName;
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


Symbol *addNewSymbolDefinition(SymbolTable *table, Symbol *symbol, Storage theDefaultStorage, UsageKind usage) {
    if (symbol == &errorSymbol || symbol->type == TypeError)
        return symbol;
    if (symbol->type == TypeError)
        return symbol;
    assert(symbol && symbol->type == TypeDefault && symbol->u.typeModifier);
    if (symbol->u.typeModifier->kind == TypeFunction && symbol->storage == StorageDefault) {
        symbol->storage = StorageExtern;
    }
    if (symbol->storage == StorageDefault) {
        symbol->storage = theDefaultStorage;
    }
    if (symbol->type == TypeDefault && symbol->storage == StorageTypedef) {
        // typedef HACK !!!
        TypeModifier *tt       = StackMemoryAlloc(TypeModifier);
        *tt                    = *symbol->u.typeModifier;
        symbol->u.typeModifier = tt;
        tt->typedefSymbol      = symbol;
    }
    if (nestingLevel() != 0) {
        // local scope symbol
        setLocalVariableLinkName(symbol);
    } else if (symbol->type == TypeDefault && symbol->storage == StorageStatic) {
        setStaticFunctionLinkName(symbol, usage);
    }
    addSymbol(table, symbol);
    addCxReference(symbol, &symbol->pos, usage, NO_FILE_NUMBER, NO_FILE_NUMBER);
    return symbol;
}

static void addInitializerRefs(Symbol *declaration, IdList *idList) {
    for (IdList *l = idList; l != NULL; l = l->next) {
        TypeModifier *typeModifierP = declaration->u.typeModifier;
        for (Id *id = &l->id; id != NULL; id = id->next) {
            if (typeModifierP->kind == TypeArray) {
                typeModifierP = typeModifierP->next;
                continue;
            }
            if (typeModifierP->kind != TypeStruct && typeModifierP->kind != TypeUnion)
                return;
            Symbol *rec = NULL;
            Reference *ref = findStructureFieldFromType(typeModifierP, id, &rec, CLASS_TO_ANY);
            if (ref == NULL)
                return;
            assert(rec);
            typeModifierP = rec->u.typeModifier;
        }
    }
}

Symbol *addNewDeclaration(SymbolTable *table, Symbol *baseType, Symbol *declaration, IdList *idList,
                          Storage storage) {
    UsageKind usageKind = UsageDefined;

    if (declaration == &errorSymbol || baseType == &errorSymbol || declaration->type == TypeError ||
        baseType->type == TypeError) {
        return declaration;
    }
    assert(declaration->type == TypeDefault);
    completeDeclarator(baseType, declaration);

    if (declaration->u.typeModifier->kind == TypeFunction)
        usageKind = UsageDeclared;
    else if (declaration->storage == StorageExtern)
        usageKind = UsageDeclared;
    addNewSymbolDefinition(table, declaration, storage, usageKind);
    addInitializerRefs(declaration, idList);
    return declaration;
}

void addFunctionParameterToSymTable(SymbolTable *table, Symbol *function, Symbol *parameter, int position) {
    if (parameter->name != NULL && parameter->type!=TypeError) {
        assert(javaStat->locals!=NULL);
        Symbol *parameterCopy = newSymbolAsCopyOf(parameter);
        Symbol *pp;

        // here checks a special case, double argument definition do not
        // redefine him, so refactorings will detect problem
        for (pp = function->u.typeModifier->u.f.args; pp != NULL && pp != parameter; pp = pp->next) {
            if (pp->name != NULL && pp->type != TypeError) {
                if (parameter != pp && strcmp(pp->name, parameter->name) == 0)
                    break;
            }
        }
        if (pp != NULL && pp != parameter) {
            Symbol *foundMember;
            if (symbolTableIsMember(table, parameterCopy, NULL, &foundMember)) {
                addCxReference(foundMember, &parameter->pos, UsageUsed, NO_FILE_NUMBER, NO_FILE_NUMBER);
            }
        } else {
            addNewSymbolDefinition(table, parameterCopy, StorageAuto, UsageDefined);
        }
        if (options.serverOperation == OLO_EXTRACT) {
            addCxReference(parameterCopy, &parameterCopy->pos, UsageLvalUsed, NO_FILE_NUMBER, NO_FILE_NUMBER);
        }
    }
    if (options.serverOperation == OLO_GOTO_PARAM_NAME
        && position == options.olcxGotoVal
        && positionsAreEqual(function->pos, cxRefPosition))
    {
        parameterPosition = parameter->pos;
    }
}

static TypeModifier *createSimpleTypeModifier(Type type) {
    TypeModifier *p;

    /* This seems to look first in pre-created types... */
    assert(type>=0 && type<MAX_TYPE);
    if (preCreatedTypesTable[type] == NULL) {
        log_trace("creating simple type %d (='%s'), *not* found in pre-created types", type,
                  typeNamesTable[type]);
        p = newSimpleTypeModifier(type);
    } else {
        log_trace("creating simple type %d (='%s'), found in pre-created types", type,
                  typeNamesTable[type]);
        p = preCreatedTypesTable[type];
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
    if (preCreatedTypesTable[t2->kind] == NULL) return t2;  /* not base type */
    if (preCreatedTypesTable[t1->kind] == NULL) return t1;  /* not base type */
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

TypeModifier *addComposedTypeToSymbol(Symbol *symbol, Type type) {
    return appendComposedType(&symbol->u.typeModifier, type);
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
    TypeModifier *typeModifier, **declaratorModifier;

    assert(type && declarator);
    if (type == &errorSymbol || declarator == &errorSymbol || type->type == TypeError
        || declarator->type == TypeError)
        return;
    declarator->storage = type->storage;
    assert(type->type == TypeDefault);
    declaratorModifier = &(declarator->u.typeModifier);
    typeModifier = type->u.typeModifier;
    if (declarator->npointers) {
        if (declarator->npointers >= 1 && (typeModifier->kind == TypeStruct || typeModifier->kind == TypeUnion)
            && typeModifier->typedefSymbol == NULL) {
            declarator->npointers--;
            assert(typeModifier->u.t && typeModifier->u.t->type == typeModifier->kind && typeModifier->u.t->u.structSpec);
            typeModifier = &typeModifier->u.t->u.structSpec->sptrtype;
        } else if (declarator->npointers >= 2 && preCreatedPtr2Ptr2TypeTable[typeModifier->kind] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers -= 2;
            typeModifier = preCreatedPtr2Ptr2TypeTable[typeModifier->kind];
        } else if (declarator->npointers >= 1 && preCreatedPtr2TypeTable[typeModifier->kind] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers--;
            typeModifier = preCreatedPtr2TypeTable[typeModifier->kind];
        }
    }
    unpackPointers(declarator);
    LIST_APPEND(TypeModifier, *declaratorModifier, typeModifier);
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
    r->storage = storage;

    return r;
}

SymbolList *createDefinitionList(Symbol *symbol) {
    SymbolList *p;

    assert(symbol);
    p = StackMemoryAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(p, symbol, NULL); with compound literal */
    *p = (SymbolList){.element = symbol, .next = NULL};

    return p;
}

Result mergeArguments(Symbol *id, Symbol *ty) {
    Result res = RESULT_OK;
    /* if a type of non-exist. argument is declared, it is probably */
    /* only a missing ';', so syntax error should be raised */
    for (; ty != NULL; ty = ty->next) {
        if (ty->name != NULL) {
            Symbol *symbol;
            for (symbol = id; symbol != NULL; symbol = symbol->next) {
                if (symbol->name != NULL && strcmp(symbol->name, ty->name) == 0)
                    break;
            }
            if (symbol == NULL)
                res = RESULT_ERR;
            else {
                if (symbol->u.typeModifier == NULL)
                    symbol->u.typeModifier = ty->u.typeModifier;
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
    symStruct->classFileNumber = -1;  /* Should be s_noFile? */
}

TypeModifier *simpleStrUnionSpecifier(Id *typeName,
                                      Id *id,
                                      UsageKind usage
) {
    Symbol symbol, *member;
    int kind;

    log_trace("new struct %s", id->name);
    assert(typeName && typeName->symbol && typeName->symbol->type == TypeKeyword);
    assert(     typeName->symbol->u.keyword == STRUCT
                ||  typeName->symbol->u.keyword == CLASS
                ||  typeName->symbol->u.keyword == UNION
                );
    if (typeName->symbol->u.keyword != UNION)
        kind = TypeStruct;
    else
        kind = TypeUnion;

    fillSymbol(&symbol, id->name, id->name, id->position);
    symbol.type = kind;
    symbol.storage = StorageNone;

    if (!symbolTableIsMember(symbolTable, &symbol, NULL, &member)
        || (isMemoryFromPreviousBlock(member) && isDefinitionOrDeclarationUsage(usage))) {
        member = StackMemoryAlloc(Symbol);
        *member = symbol;
        member->u.structSpec = StackMemoryAlloc(S_symStructSpec);

        initSymStructSpec(member->u.structSpec, /*.records=*/NULL);
        TypeModifier *stype = &member->u.structSpec->stype;
        /* Assumed to be Struct/Union/Enum? */
        initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/kind, /*.u.t=*/member,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
        TypeModifier *sptrtype = &member->u.structSpec->sptrtype;
        initTypeModifierAsPointer(sptrtype, &member->u.structSpec->stype);

        setGlobalFileDepNames(id->name, member, MEMORY_XX);
        addSymbol(symbolTable, member);
    }
    addCxReference(member, &id->position, usage,NO_FILE_NUMBER, NO_FILE_NUMBER);
    return &member->u.structSpec->stype;
}

void setGlobalFileDepNames(char *iname, Symbol *symbol, int memory) {
    char *mname, *fname;
    char tmp[MACRO_NAME_SIZE];
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
        Symbol *member;
        // anonymous enum/structure/union ...
        int fileNumber = symbol->pos.file;
        symbol->name=iname;
        symbol->linkName=iname;
        order = 0;
        isMember = symbolTableIsMember(symbolTable, symbol, NULL, &member);
        while (isMember) {
            if (member->pos.file==fileNumber)
                order++;
            isMember = symbolTableNextMember(symbol, &member);
        }
        fname = simpleFileName(getFileItem(fileNumber)->name);
        sprintf(tmp, "%s%c%d%c", fname, FILE_PATH_SEPARATOR, order, LINK_NAME_SEPARATOR);
        /*&     // macros will be identified by name only?
          } else if (symbol->type == TypeMacro) {
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
        mname = ppmAllocc(len2+1, sizeof(char));
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
    assert(typeName->symbol->type == TypeKeyword);
    assert(typeName->symbol->u.keyword == STRUCT
           ||  typeName->symbol->u.keyword == CLASS
           ||  typeName->symbol->u.keyword == UNION
           );
    if (typeName->symbol->u.keyword == STRUCT) type = TypeStruct;
    else type = TypeUnion;

    pp = newSymbol("", NULL, typeName->position);
    pp->type = type;
    pp->storage = StorageNone;

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

    addSymbol(symbolTable, pp);

    return &pp->u.structSpec->stype;
}

void specializeStrUnionDef(Symbol *sd, Symbol *rec) {
    Symbol *dd;
    assert(sd->type == TypeStruct || sd->type == TypeUnion);
    assert(sd->u.structSpec);
    if (sd->u.structSpec->records!=NULL) return;
    sd->u.structSpec->records = rec;
    addToTrail(setToNull, & (sd->u.structSpec->records), (LANGUAGE(LANG_C)||LANGUAGE(LANG_YACC)));
    for(dd=rec; dd!=NULL; dd=dd->next) {
        if (dd->name!=NULL) {
            dd->linkName = string3ConcatInStackMem(sd->linkName,".",dd->name);
            addCxReference(dd,&dd->pos,UsageDefined,NO_FILE_NUMBER, NO_FILE_NUMBER);
        }
    }
}

TypeModifier *simpleEnumSpecifier(Id *id, UsageKind usage) {
    Symbol p, *pp;

    fillSymbol(&p, id->name, id->name, id->position);
    p.type = TypeEnum;
    p.storage = StorageNone;

    if (! symbolTableIsMember(symbolTable, &p, NULL, &pp)
        || (isMemoryFromPreviousBlock(pp) && isDefinitionOrDeclarationUsage(usage))) {
        pp = StackMemoryAlloc(Symbol);
        *pp = p;
        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbol(symbolTable, pp);
    }
    addCxReference(pp, &id->position, usage,NO_FILE_NUMBER, NO_FILE_NUMBER);
    return createSimpleEnumType(pp);
}

TypeModifier *createNewAnonymousEnum(SymbolList *enums) {
    Symbol *symbol;

    symbol = newSymbolAsEnum("", "", noPosition, enums);
    symbol->type = TypeEnum;
    symbol->storage = StorageNone;

    setGlobalFileDepNames("", symbol, MEMORY_XX);
    symbol->u.enums = enums;
    return createSimpleEnumType(symbol);
}

void appendPositionToList(PositionList **list, Position *pos) {
    PositionList *ppl;
    ppl = newPositionList(*pos, NULL);
    LIST_APPEND(PositionList, (*list), ppl);
}

void setParamPositionForFunctionWithoutParams(Position *lpar) {
    parameterBeginPosition = *lpar;
    parameterEndPosition = *lpar;
}

void setParamPositionForParameter0(Position *lpar) {
    parameterBeginPosition = *lpar;
    parameterEndPosition = *lpar;
}

void setParamPositionForParameterBeyondRange(Position *rpar) {
    parameterBeginPosition = *rpar;
    parameterEndPosition = *rpar;
}

static void handleParameterPositions(Position *lpar, PositionList *commas, Position *rpar, bool hasParam, bool isVoid) {
    int           i, argn;
    Position     *p1, *p2;
    PositionList *list;

    /* Sets the following global variables as "answer":
       - parameterListIsVoid
       - parameterCount
       - parameterBeginPosition
       - parameterEndPosition
    */

    parameterListIsVoid = isVoid;
    parameterCount = 0;

    argn = options.olcxGotoVal;

    if (!hasParam) {
        setParamPositionForFunctionWithoutParams(lpar);
        return;
    }

    if (!isVoid) {
        LIST_LEN(parameterCount, PositionList, commas);
        parameterCount++;
    }

    assert(argn > 0);           /* TODO: WTF is Parameter0 and when is it used? */
    if (argn == 0) {
        setParamPositionForParameter0(lpar);
    } else {
        list = commas;
        p1 = lpar;
        if (list != NULL)
            p2 = &list->position;
        else
            p2 = rpar;
        for (i=2; list != NULL && i <= argn; list = list->next, i++) {
            p1 = &list->position;
            if (list->next != NULL)
                p2 = &list->next->position;
            else
                p2 = rpar;
        }
        if (list == NULL && i <= argn) {
            setParamPositionForParameterBeyondRange(rpar);
        } else {
            parameterBeginPosition = *p1;
            parameterEndPosition   = *p2;
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
                                    bool hasParam, bool isVoid
                                    ) {
    if (options.mode != ServerMode)
        return;
    if (options.serverOperation != OLO_GOTO_PARAM_NAME && options.serverOperation != OLO_GET_PARAM_COORDINATES)
        return;
    if (positionsAreNotEqual(decl->pos, cxRefPosition))
        return;
    handleParameterPositions(lpar, commas, rpar, hasParam, isVoid);
}

void handleInvocationParamPositions(Reference *ref, Position *lpar,
                                    PositionList *commas, Position *rpar,
                                    bool hasParam
                                    ) {
    if (options.mode != ServerMode)
        return;
    if (options.serverOperation != OLO_GOTO_PARAM_NAME && options.serverOperation != OLO_GET_PARAM_COORDINATES)
        return;
    if (ref==NULL || positionsAreNotEqual(ref->position, cxRefPosition))
        return;
    handleParameterPositions(lpar, commas, rpar, hasParam, false);
}

void javaHandleDeclaratorParamPositions(Position *sym, Position *lpar,
                                        PositionList *commas, Position *rpar
                                        ) {
    if (options.mode != ServerMode)
        return;
    if (options.serverOperation != OLO_GOTO_PARAM_NAME && options.serverOperation != OLO_GET_PARAM_COORDINATES)
        return;
    if (positionsAreNotEqual(*sym, cxRefPosition))
        return;
    if (commas==NULL) {
        handleParameterPositions(lpar, NULL, rpar, false, false);
    } else {
        handleParameterPositions(lpar, commas->next, rpar, true, false);
    }
}
