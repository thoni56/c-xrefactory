#include "semact.h"

#include "commons.h"
#include "counters.h"
#include "globals.h"
#include "filedescriptor.h"
#include "options.h"
#include "misc.h"
#include "proto.h"
#include "usage.h"
#include "yylex.h"
#include "cxref.h"
#include "symbol.h"
#include "list.h"
#include "filetable.h"
#include "stackmemory.h"

#include "hash.h"
#include "log.h"


void fillRecFindStr(S_recFindStr *recFindStr, Symbol *baseClass, Symbol *currentClass, Symbol *nextRecord,
                    unsigned recsClassCounter) {
    recFindStr->baseClass            = baseClass;
    assert(currentClass == NULL); /* Current class == NULL for C/Yacc */
    recFindStr->currentClass         = currentClass;
    recFindStr->nextRecord           = nextRecord;
    recFindStr->recsClassCounter     = recsClassCounter;
    recFindStr->superClassesCount    = 0;
    recFindStr->anonymousUnionsCount = 0;
}

bool displayingErrorMessages(void) {
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


/* Used as action in addToFrame(), thus void* argument */
void setToNull(void *p) {
    void **pp;
    pp = (void **)p;
    *pp = NULL;
}


/* Used as action in addToFrame(), thus void* argument */
void deleteSymDef(void *p) {
    Symbol        *pp;

    pp = (Symbol *) p;
    log_debug("deleting %s %s", pp->name, pp->linkName);
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

void addSymbolToFrame(SymbolTable *table, Symbol *symbol) {
    /*  A bug can happen if you add a symbol into old table, and the
        same symbol exists in a newer one. Then it will be deleted
        from the newer one. All this story is about storing
        information in frameAllocations. It should contain both
        table and pointer !!!!
    */
    log_debug("adding symbol %s: %s %s", symbol->name, typeNamesTable[symbol->type],
              storageEnumName[symbol->storage]);
    assert(symbol->npointers==0);
    addSymbolToTable(table, symbol);
    addToFrame(deleteSymDef, symbol /* TODO? Should also include reference to table */);
}

void recFindPush(Symbol *symbol, S_recFindStr *rfs) {
    S_symStructSpec *ss;

    assert(symbol && (symbol->type==TypeStruct || symbol->type==TypeUnion));
    if (rfs->recsClassCounter==0) {
        // this is hack to avoid problem when overloading to zero
        rfs->recsClassCounter++;
    }
    ss = symbol->u.structSpec;
    rfs->nextRecord = ss->records;
    rfs->currentClass                         = symbol;
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
        if (typeModifier->type==TypePointer || typeModifier->type==TypeArray) {
            s_structRecordCompletionType = typeModifier->next;
            assert(s_structRecordCompletionType);
        } else s_structRecordCompletionType = &errorModifier;
    }
}

Result findStrRecordSym(Symbol **resultingSymbolP, S_recFindStr *ss, char *recname,
                        AccessibilityCheckYesNo accessibilityCheck, /* java check accessibility */
                        VisibilityCheckYesNo    visibilityCheck     /* redundant, always equal to accCheck? */
) {
    Symbol     *s, *r, *cclass;
    SymbolList *sss;

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
                r->u.typeModifier->type == TypeAnonymousField && r->u.typeModifier->next != NULL &&
                (r->u.typeModifier->next->type == TypeUnion || r->u.typeModifier->next->type == TypeStruct)) {
                // put the anonymous union as 'super class'
                if (ss->anonymousUnionsCount + 1 < MAX_ANONYMOUS_FIELDS) {
                    ss->anonymousUnions[ss->anonymousUnionsCount++] = r->u.typeModifier->next->u.t;
                }
            }
            //&fprintf(dumpOut,":checking %s\n",r->name); fflush(dumpOut);
            if (recname == NULL || strcmp(r->name, recname) == 0) {
                *resultingSymbolP = r;
                ss->nextRecord = r->next;
                return RESULT_OK;
            }
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
                *resultingSymbolP = &errorSymbol;
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

int findStrRecord(Symbol *symbol,
                  char *recname,   /* can be NULL */
                  Symbol **res) {
    S_recFindStr rfs;
    return findStrRecordSym(res, iniFind(symbol,&rfs), recname,
                           ACCESSIBILITY_CHECK_YES, VISIBILITY_CHECK_YES);
}

/* and push reference */
// this should be split into two copies, different for C and Java.
Reference *findStrRecordFromSymbol(Symbol *sym,
                                   Id *record,
                                   Symbol **res,
                                   Id *super /* covering special case when invoked
                                                as SUPER.sym, berk */
) {
    S_recFindStr    rfs;
    Reference     *ref;

    ref = NULL;
    // when in java, then always in qualified name, so access and visibility checks
    // are useless.
    Result rr = findStrRecordSym(res, iniFind(sym,&rfs),record->name, ACCESSIBILITY_CHECK_NO, VISIBILITY_CHECK_NO);
    if (rr == RESULT_OK) {
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

    assert(javaClassifier == CLASS_TO_ANY);
    assert(structure);
    if (structure->type != TypeStruct && structure->type != TypeUnion) {
        *resultingSymbol = &errorSymbol;
        goto fini;
    }
    reference = findStrRecordFromSymbol(structure->u.t, field, resultingSymbol, NULL);
 fini:
    return reference;
}

void labelReference(Id *id, UsageKind usage) {
    char tempString[TMP_STRING_SIZE];
    char *t;
    assert(id);
    if (parsedClassInfo.function!=NULL) {
        t = strmcpy(tempString, parsedClassInfo.function->name);
        *t = '.';
        t = strcpy(t+1,id->name);
    } else {
        strcpy(tempString, id->name);
    }
    assert(strlen(tempString)<TMP_STRING_SIZE-1);
    addTrivialCxReference(tempString, TypeLabel,StorageDefault, id->position, usage);
}

void generateInternalLabelReference(int counter, int usage) {
    char labelName[TMP_STRING_SIZE];
    Id labelId;
    Position position;

    if (options.serverOperation != OLO_EXTRACT)
        return;

    snprintf(labelName, TMP_STRING_SIZE, "%%L%d", counter);

    position = (Position){.file = currentFile.characterBuffer.fileNumber, .line = 0, .col = 0};
    fillId(&labelId, labelName, NULL, position);

    if (usage != UsageDefined)
        labelId.position.line++;
    // line == 0 or 1 , (hack to get definition first)
    labelReference(&labelId, usage);
}

void setLocalVariableLinkName(struct symbol *p) {
    char ttt[TMP_STRING_SIZE];
    char nnn[TMP_STRING_SIZE];
    int len,tti;
    if (options.serverOperation == OLO_EXTRACT) {
        // extract variable, must pass all needed informations in linkname
        sprintf(nnn, "%c%s%c", LINK_NAME_SEPARATOR, p->name, LINK_NAME_SEPARATOR);
        ttt[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
        sprintf(ttt+1,"%s", storageNamesTable[p->storage]);
        tti = strlen(ttt);
        len = TMP_STRING_SIZE - tti;
        typeSPrint(ttt+tti, &len, p->u.typeModifier, nnn, LINK_NAME_SEPARATOR, 0, true, SHORT_NAME, NULL);
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
    p->linkName = stackMemoryAlloc(len+1);
    strcpy(p->linkName,ttt);
}

static void setStaticFunctionLinkName(Symbol *p, char *fileName, int usage) {
    char        ttt[TMP_STRING_SIZE];
    int         len;
    char        *ss, *basefname;

    //& if (! symbolTableIsMember(symbolTable, p, &ii, &memb)) {
    // follwing unifies static symbols taken from the same header files.
    // Static symbols can be used only after being defined, so it is sufficient
    // to do this on definition usage?
    // With exactPositionResolve interpret them as distinct symbols for
    // each compilation unit.
    if (usage==UsageDefined && ! options.exactPositionResolve) {
        basefname=getFileItem(p->pos.file)->name;
    } else {
        basefname=fileName;
    }
    sprintf(ttt,"%s!%s", simpleFileName(basefname), p->name);
    len = strlen(ttt);
    assert(len < TMP_STRING_SIZE-2);
    ss = stackMemoryAlloc(len+1);
    strcpy(ss, ttt);
    p->linkName = ss;
    //& } else {
    //&     p->linkName=memb->linkName;
    //& }
}


Symbol *addNewSymbolDefinition(SymbolTable *table, char *fileName, Symbol *symbol, Storage theDefaultStorage, UsageKind usage) {
    if (symbol == &errorSymbol || symbol->type == TypeError)
        return symbol;
    if (symbol->type == TypeError)
        return symbol;
    assert(symbol && symbol->type == TypeDefault && symbol->u.typeModifier);
    if (symbol->u.typeModifier->type == TypeFunction && symbol->storage == StorageDefault) {
        symbol->storage = StorageExtern;
    }
    if (symbol->storage == StorageDefault) {
        symbol->storage = theDefaultStorage;
    }
    if (symbol->type == TypeDefault && symbol->storage == StorageTypedef) {
        // typedef HACK !!!
        TypeModifier *tt       = stackMemoryAlloc(sizeof(TypeModifier));
        *tt                    = *symbol->u.typeModifier;
        symbol->u.typeModifier = tt;
        tt->typedefSymbol      = symbol;
    }
    if (nestingLevel() != 0) {
        // local scope symbol
        setLocalVariableLinkName(symbol);
    } else if (symbol->type == TypeDefault && symbol->storage == StorageStatic) {
        setStaticFunctionLinkName(symbol, fileName, usage);
    }
    addSymbolToFrame(table, symbol);
    addCxReference(symbol, &symbol->pos, usage, NO_FILE_NUMBER, NO_FILE_NUMBER);
    return symbol;
}

static void addInitializerRefs(Symbol *declaration, IdList *idList) {
    for (IdList *l = idList; l != NULL; l = l->next) {
        TypeModifier *typeModifierP = declaration->u.typeModifier;
        for (Id *id = &l->id; id != NULL; id = id->next) {
            if (typeModifierP->type == TypeArray) {
                typeModifierP = typeModifierP->next;
                continue;
            }
            if (typeModifierP->type != TypeStruct && typeModifierP->type != TypeUnion)
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

    if (declaration->u.typeModifier->type == TypeFunction)
        usageKind = UsageDeclared;
    else if (declaration->storage == StorageExtern)
        usageKind = UsageDeclared;
    addNewSymbolDefinition(table, inputFileName, declaration, storage, usageKind);
    addInitializerRefs(declaration, idList);
    return declaration;
}

void addFunctionParameterToSymTable(SymbolTable *table, Symbol *function, Symbol *parameter, int position) {
    if (parameter->name != NULL && parameter->type!=TypeError) {
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
            addNewSymbolDefinition(table, inputFileName, parameterCopy, StorageAuto, UsageDefined);
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
    assert(p->type == type);

    return p;
}

static TypeModifier *mergeBaseType(TypeModifier *t1,TypeModifier *t2){
    unsigned b,r;
    unsigned modif;
    assert(t1->type<TYPE_MODIFIERS_END && t2->type<TYPE_MODIFIERS_END);
    b=t1->type; modif=t2->type;// just to confuse compiler warning
    /* if both are types, error, return the new one only*/
    if (t1->type <= MODIFIERS_START && t2->type <= MODIFIERS_START)
        return t2;
    /* if not use tables*/
    if (t1->type > MODIFIERS_START) {modif = t1->type; b = t2->type; }
    if (t2->type > MODIFIERS_START) {modif = t2->type; b = t1->type; }
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
    if (t1->type == TypeDefault) return t2;
    if (t2->type == TypeDefault) return t1;
    assert(t1->type >=0 && t1->type<MAX_TYPE);
    assert(t2->type >=0 && t2->type<MAX_TYPE);
    if (preCreatedTypesTable[t2->type] == NULL) return t2;  /* not base type */
    if (preCreatedTypesTable[t1->type] == NULL) return t1;  /* not base type */
    return mergeBaseType(t1, t2);
}

Symbol *typeSpecifier2(TypeModifier *t) {
    Symbol    *r;

    r = stackMemoryAlloc(sizeof(Symbol));
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
        if (declarator->npointers >= 1 && (typeModifier->type == TypeStruct || typeModifier->type == TypeUnion)
            && typeModifier->typedefSymbol == NULL) {
            declarator->npointers--;
            assert(typeModifier->u.t && typeModifier->u.t->type == typeModifier->type && typeModifier->u.t->u.structSpec);
            typeModifier = &typeModifier->u.t->u.structSpec->ptrtype;
        } else if (declarator->npointers >= 2 && preCreatedPtr2Ptr2TypeTable[typeModifier->type] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers -= 2;
            typeModifier = preCreatedPtr2Ptr2TypeTable[typeModifier->type];
        } else if (declarator->npointers >= 1 && preCreatedPtr2TypeTable[typeModifier->type] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers--;
            typeModifier = preCreatedPtr2TypeTable[typeModifier->type];
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
    p = stackMemoryAlloc(sizeof(SymbolList));
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
    Symbol *member;

    log_trace("new struct %s", id->name);
    assert(typeName && typeName->symbol && typeName->symbol->type == TypeKeyword);
    assert(typeName->symbol->u.keyword == STRUCT
           ||  typeName->symbol->u.keyword == CLASS
           ||  typeName->symbol->u.keyword == UNION);

    Type type;
    if (typeName->symbol->u.keyword != UNION)
        type = TypeStruct;
    else
        type = TypeUnion;

    Symbol symbol = makeSymbol(id->name, id->name, id->position);
    symbol.type = type;
    symbol.storage = StorageNone;

    if (!symbolTableIsMember(symbolTable, &symbol, NULL, &member)
        || (isMemoryFromPreviousBlock(member) && isDefinitionOrDeclarationUsage(usage))) {
        member = stackMemoryAlloc(sizeof(Symbol));
        *member = symbol;
        member->u.structSpec = stackMemoryAlloc(sizeof(S_symStructSpec));

        initSymStructSpec(member->u.structSpec, NULL);

        TypeModifier *typeModifier = &member->u.structSpec->type;
        /* Assumed to be Struct/Union/Enum? */
        initTypeModifierAsStructUnionOrEnum(typeModifier, type, member, NULL, NULL);

        TypeModifier *ptrtypeModifier = &member->u.structSpec->ptrtype;
        initTypeModifierAsPointer(ptrtypeModifier, &member->u.structSpec->type);

        setGlobalFileDepNames(id->name, member, MEMORY_XX);
        addSymbolToFrame(symbolTable, member);
    }
    addCxReference(member, &id->position, usage,NO_FILE_NUMBER, NO_FILE_NUMBER);
    return &member->u.structSpec->type;
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
        mname = stackMemoryAlloc(len2+1);
    } else {
        mname = ppmAllocc(len2+1, sizeof(char));
    }
    strcpy(mname, tmp);
    strcpy(mname+len,iname);
    symbol->name = mname + len;
    symbol->linkName = mname;
}

TypeModifier *createNewAnonymousStructOrUnion(Id *typeName) {
    Symbol *symbol;
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

    symbol = newSymbol("", NULL, typeName->position);
    symbol->type = type;
    symbol->storage = StorageNone;

    setGlobalFileDepNames("", symbol, MEMORY_XX);

    symbol->u.structSpec = stackMemoryAlloc(sizeof(S_symStructSpec));

    /* This is a recurring pattern, create a struct and the pointer type to it*/
    initSymStructSpec(symbol->u.structSpec, /*.records=*/NULL);
    TypeModifier *stype = &symbol->u.structSpec->type;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/symbol,
                                        /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *sptrtype = &symbol->u.structSpec->ptrtype;
    initTypeModifierAsPointer(sptrtype, &symbol->u.structSpec->type);

    addSymbolToFrame(symbolTable, symbol);

    return &symbol->u.structSpec->type;
}

void specializeStrUnionDef(Symbol *sd, Symbol *rec) {
    assert(sd->type == TypeStruct || sd->type == TypeUnion);
    assert(sd->u.structSpec);
    if (sd->u.structSpec->records!=NULL)
        return;

    sd->u.structSpec->records = rec;
    addToFrame(setToNull, &(sd->u.structSpec->records));
    for (Symbol *symbol=rec; symbol!=NULL; symbol=symbol->next) {
        if (symbol->name != NULL) {
            symbol->linkName = string3ConcatInStackMem(sd->linkName,".",symbol->name);
            addCxReference(symbol,&symbol->pos,UsageDefined,NO_FILE_NUMBER, NO_FILE_NUMBER);
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
        pp = stackMemoryAlloc(sizeof(Symbol));
        *pp = p;
        setGlobalFileDepNames(id->name, pp, MEMORY_XX);
        addSymbolToFrame(symbolTable, pp);
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
