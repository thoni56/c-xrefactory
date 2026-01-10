#include "semact.h"

#include <string.h>

#include "commons.h"
#include "counters.h"
#include "globals.h"
#include "filedescriptor.h"
#include "options.h"
#include "parsing.h"
#include "misc.h"
#include "position.h"
#include "proto.h"
#include "storage.h"
#include "type.h"
#include "usage.h"
#include "yylex.h"
#include "cxref.h"
#include "symbol.h"
#include "list.h"
#include "filetable.h"
#include "stackmemory.h"

#include "hash.h"
#include "log.h"


#define MACRO_NAME_SIZE 500


void fillStructMemberFindInfo(StructMemberFindInfo *info, Symbol *currentStructure,
                              Symbol *nextRecord, unsigned memberFindCount) {
    info->currentStructure = currentStructure;
    info->nextMember = nextRecord;
    info->memberFindCount = memberFindCount;
    info->anonymousUnionsCount = 0;
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
            log_debug("Syntax error on: '%s'", yytext);
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

void noSuchMemberError(char *memberName) {
    char message[TMP_BUFF_SIZE];
    if (options.debug || options.errors) {
        sprintf(message, "Field/member '%s' not found", memberName);
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
        appendComposedType(&symbol->typeModifier, TypePointer);
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

void memberFindPush(Symbol *symbol, StructMemberFindInfo *info) {
    StructSpec *spec;

    assert(symbol && (symbol->type==TypeStruct || symbol->type==TypeUnion));
    if (info->memberFindCount==0) {
        // this is hack to avoid problem when overloading to zero
        info->memberFindCount++;
    }
    spec = symbol->structSpec;
    info->nextMember = spec->members;
    info->currentStructure = symbol;
}

StructMemberFindInfo *initFind(Symbol *s, StructMemberFindInfo *info) {
    assert(s);
    assert(s->type == TypeStruct || s->type == TypeUnion);
    assert(s->structSpec);
    assert(info);
    fillStructMemberFindInfo(info, NULL, NULL, memberFindCount++);
    memberFindPush(s, info);
    return info;
}

void setDirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.mode);
    if (options.mode == ServerMode) {
        structMemberCompletionType = typeModifier;
        assert(structMemberCompletionType);
    }
}

void setIndirectStructureCompletionType(TypeModifier *typeModifier) {
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (typeModifier->type==TypePointer || typeModifier->type==TypeArray) {
            structMemberCompletionType = typeModifier->next;
            assert(structMemberCompletionType);
        } else structMemberCompletionType = &errorModifier;
    }
}

Result findStructureMemberSymbol(Symbol **resultingSymbolP, StructMemberFindInfo *info, char *memberName) {
    Symbol     *symbol, *structure;

    for (;;) {
        assert(info);
        structure = info->currentStructure;
        if (structure != NULL && structure->structSpec->memberSearchCounter == info->memberFindCount) {
            goto nextStruct;
        }

        for (Symbol *m = info->nextMember; m != NULL; m = m->next) {
            // special gcc extension of anonymous struct field/member?
            // typedef struct {
            //   struct {
            //     int x, y;
            //   }; // Anonymous struct
            //   double radius;
            // } Circle;
            if (m->name != NULL && *m->name == 0 && m->type == TypeDefault &&
                m->typeModifier->type == TypeAnonymousField && m->typeModifier->next != NULL &&
                (m->typeModifier->next->type == TypeUnion || m->typeModifier->next->type == TypeStruct)
            ) {
                if (info->anonymousUnionsCount + 1 < MAX_ANONYMOUS_FIELDS) {
                    info->anonymousUnions[info->anonymousUnionsCount++] = m->typeModifier->next->typeSymbol;
                }
            }
            //&fprintf(dumpOut,":checking %s\n",m->name); fflush(dumpOut);
            if (memberName == NULL || strcmp(m->name, memberName) == 0) {
                *resultingSymbolP = m;
                info->nextMember = m->next;
                return RESULT_OK;
            }
        }
    nextStruct:
        if (info->anonymousUnionsCount != 0) {
            // O.K. try first to pass down to anonymous record
            symbol = info->anonymousUnions[--info->anonymousUnionsCount];
        } else {
            // mark the struct as processed
            if (structure != NULL) {
                structure->structSpec->memberSearchCounter = info->memberFindCount;
            }

            info->nextMember = NULL;
            *resultingSymbolP = &errorSymbol;
            return RESULT_NOT_FOUND;
        }
        memberFindPush(symbol, info);
    }
}

int findStructureMember(Symbol *symbol, char *memberName, /* can be NULL */
                        Symbol **foundSymbol
) {
    StructMemberFindInfo info;
    return findStructureMemberSymbol(foundSymbol, initFind(symbol, &info), memberName);
}

Reference *findStuctureMemberFromSymbol(Symbol *symbol, Id *member, Symbol **resultingMemberSymbol) {
    StructMemberFindInfo info;
    Reference *ref = NULL;
    Result result = findStructureMemberSymbol(resultingMemberSymbol, initFind(symbol, &info), member->name);

    if (result == RESULT_OK) {
        ref = handleFoundSymbolReference(*resultingMemberSymbol, member->position, UsageUsed, NO_FILE_NUMBER);
    } else {
        noSuchMemberError(member->name);
    }
    return ref;
}

Reference *findStructureFieldFromType(TypeModifier *structure,
                                        Id *field,
                                        Symbol **resultingSymbol) {
    Reference *reference = NULL;

    assert(structure);
    if (structure->type != TypeStruct && structure->type != TypeUnion) {
        *resultingSymbol = &errorSymbol;
        goto fini;
    }
    reference = findStuctureMemberFromSymbol(structure->typeSymbol, field, resultingSymbol);
 fini:
    return reference;
}

void labelReference(Id *id,  Usage usage) {
    assert(id);

    char tempString[TMP_STRING_SIZE];
    if (parsedInfo.function!=NULL) {
        char *t = strmcpy(tempString, parsedInfo.function->name);
        *t = '.';
        t = strcpy(t+1,id->name);
    } else {
        strcpy(tempString, id->name);
    }
    assert(strlen(tempString)<TMP_STRING_SIZE-1);
    addTrivialCxReference(tempString, TypeLabel, StorageDefault, id->position, usage);
}

void generateInternalLabelReference(int counter, int usage) {
    if (parsingConfig.operation != PARSE_TO_EXTRACT)
        return;

    char labelName[TMP_STRING_SIZE];
    snprintf(labelName, TMP_STRING_SIZE, "%%L%d", counter);

    Position position = (Position){.file = currentFile.characterBuffer.fileNumber, .line = 0, .col = 0};
    Id labelId = makeId(labelName, NULL, position);

    if (usage != UsageDefined)
        labelId.position.line++;
    // line == 0 or 1 , (hack to get definition first)
    labelReference(&labelId, usage);
}

void setLocalVariableLinkName(Symbol *p) {
    char name[TMP_STRING_SIZE];
    if (parsingConfig.operation == PARSE_TO_EXTRACT) {
        char nnn[TMP_STRING_SIZE];
        // extract variable, must pass all needed informations in linkname
        sprintf(nnn, "%c%s%c", LINK_NAME_SEPARATOR, p->name, LINK_NAME_SEPARATOR);
        name[0] = LINK_NAME_EXTRACT_DEFAULT_FLAG;
        sprintf(name+1,"%s", storageNamesTable[p->storage]);
        int tti = strlen(name);
        int len = TMP_STRING_SIZE - tti;
        prettyPrintType(name+tti, &len, p->typeModifier, nnn, LINK_NAME_SEPARATOR, true);
        sprintf(name+tti+len,"%c%x-%x-%x-%x", LINK_NAME_SEPARATOR,
                p->position.file,p->position.line,p->position.col, nextGeneratedLocalSymbol());
    } else {
        if (p->storage==StorageExtern && ! options.exactPositionResolve) {
            sprintf(name,"%s", p->name);
        } else {
            sprintf(name,"%x-%x-%x%c%s",p->position.file,p->position.line,p->position.col,
                    LINK_NAME_SEPARATOR, p->name);
        }
    }
    p->linkName = stackMemoryAlloc(strlen(name)+1);
    strcpy(p->linkName,name);
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
        basefname=getFileItemWithFileNumber(p->position.file)->name;
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


Symbol *addNewSymbolDefinition(SymbolTable *table, char *fileName, Symbol *symbol, Storage theDefaultStorage,
                               Usage usage) {
    if (symbol == &errorSymbol || symbol->type == TypeError)
        return symbol;
    assert(symbol && symbol->type == TypeDefault && symbol->typeModifier);
    if (symbol->typeModifier->type == TypeFunction && symbol->storage == StorageDefault) {
        symbol->storage = StorageExtern;
    }
    if (symbol->storage == StorageDefault) {
        symbol->storage = theDefaultStorage;
    }
    if (symbol->type == TypeDefault && symbol->storage == StorageTypedef) {
        // typedef HACK !!!
        TypeModifier *tt       = stackMemoryAlloc(sizeof(TypeModifier));
        *tt                    = *symbol->typeModifier;
        symbol->typeModifier = tt;
        tt->typedefSymbol      = symbol;
    }
    if (nestingLevel() != 0) {
        // local scope symbol
        setLocalVariableLinkName(symbol);
    } else if (symbol->type == TypeDefault && symbol->storage == StorageStatic) {
        setStaticFunctionLinkName(symbol, fileName, usage);
    }
    addSymbolToFrame(table, symbol);
    handleFoundSymbolReference(symbol, symbol->position, usage, NO_FILE_NUMBER);
    return symbol;
}

static void addInitializerRefs(Symbol *declaration, IdList *idList) {
    for (IdList *l = idList; l != NULL; l = l->next) {
        TypeModifier *typeModifierP = declaration->typeModifier;
        for (Id *id = &l->id; id != NULL; id = id->next) {
            if (typeModifierP->type == TypeArray) {
                typeModifierP = typeModifierP->next;
                continue;
            }
            if (typeModifierP->type != TypeStruct && typeModifierP->type != TypeUnion)
                return;
            Symbol *rec = NULL;
            Reference *ref = findStructureFieldFromType(typeModifierP, id, &rec);
            if (ref == NULL)
                return;
            assert(rec);
            typeModifierP = rec->typeModifier;
        }
    }
}

Symbol *addNewDeclaration(SymbolTable *table, Symbol *baseType, Symbol *declaration, IdList *idList,
                          Storage storage) {
     Usage usage = UsageDefined;

    if (declaration == &errorSymbol || baseType == &errorSymbol || declaration->type == TypeError ||
        baseType->type == TypeError) {
        return declaration;
    }
    assert(declaration->type == TypeDefault);
    completeDeclarator(baseType, declaration);

    if (declaration->typeModifier->type == TypeFunction)
        usage = UsageDeclared;
    else if (declaration->storage == StorageExtern)
        usage = UsageDeclared;
    addNewSymbolDefinition(table, parsingConfig.fileName, declaration, storage, usage);
    addInitializerRefs(declaration, idList);
    return declaration;
}

void addFunctionParameterToSymbolTable(SymbolTable *table, Symbol *function, Symbol *parameter,
                                       int parameterIndex) {
    if (parameter->name != NULL && parameter->type!=TypeError) {
        Symbol *parameterCopy = newSymbolAsCopyOf(parameter);
        Symbol *pp;

        // here checks a special case, double argument definition do not
        // redefine him, so refactorings will detect problem
        for (pp = function->typeModifier->args; pp != NULL && pp != parameter; pp = pp->next) {
            if (pp->name != NULL && pp->type != TypeError) {
                if (parameter != pp && strcmp(pp->name, parameter->name) == 0)
                    break;
            }
        }
        if (pp != NULL && pp != parameter) {
            Symbol *foundMember;
            if (symbolTableIsMember(table, parameterCopy, NULL, &foundMember)) {
                handleFoundSymbolReference(foundMember, parameter->position, UsageUsed, NO_FILE_NUMBER);
            }
        } else {
            addNewSymbolDefinition(table, parsingConfig.fileName, parameterCopy, StorageAuto, UsageDefined);
        }
        if (parsingConfig.operation == PARSE_TO_EXTRACT) {
            handleFoundSymbolReference(parameterCopy, parameterCopy->position, UsageLvalUsed, NO_FILE_NUMBER);
        }
    }
    if (parsingConfig.operation == PARSE_TO_TRACK_PARAMETERS
        && parameterIndex == parsingConfig.targetParameterIndex
        && positionsAreEqual(function->position, parsingConfig.positionOfSelectedReference))
    {
        parameterPosition = parameter->position;
    }
}

static TypeModifier *createSimpleTypeModifier(Type type) {
    TypeModifier *p;

    /* This seems to look first in pre-created types... */
    assert(type>=0 && type<MAX_TYPE);
    if (builtinTypesTable[type] == NULL) {
        log_debug("creating simple type %d (='%s'), *not* found in pre-created types", type,
                  typeNamesTable[type]);
        p = newSimpleTypeModifier(type);
    } else {
        log_debug("creating simple type %d (='%s'), found in pre-created types", type,
                  typeNamesTable[type]);
        p = builtinTypesTable[type];
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
    if (builtinTypesTable[t2->type] == NULL) return t2;  /* not base type */
    if (builtinTypesTable[t1->type] == NULL) return t1;  /* not base type */
    return mergeBaseType(t1, t2);
}

Symbol *typeSpecifier2(TypeModifier *t) {
    Symbol    *r;

    r = stackMemoryAlloc(sizeof(Symbol));
    fillSymbolWithTypeModifier(r, NULL, NULL, NO_POSITION, t);

    return r;
}

Symbol *typeSpecifier1(unsigned t) {
    Symbol        *r;
    r = typeSpecifier2(createSimpleTypeModifier(t));
    return r;
}

void declTypeSpecifier1(Symbol *d, Type type) {
    assert(d && d->typeModifier);
    d->typeModifier = mergeBaseModTypes(d->typeModifier,createSimpleTypeModifier(type));
}

void declTypeSpecifier2(Symbol *d, TypeModifier *t) {
    assert(d && d->typeModifier);
    d->typeModifier = mergeBaseModTypes(d->typeModifier, t);
}

TypeModifier *addComposedTypeToSymbol(Symbol *symbol, Type type) {
    return appendComposedType(&symbol->typeModifier, type);
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
    declaratorModifier = &(declarator->typeModifier);
    typeModifier = type->typeModifier;
    if (declarator->npointers) {
        if (declarator->npointers >= 1 && (typeModifier->type == TypeStruct || typeModifier->type == TypeUnion)
            && typeModifier->typedefSymbol == NULL) {
            declarator->npointers--;
            assert(typeModifier->typeSymbol && typeModifier->typeSymbol->type == typeModifier->type && typeModifier->typeSymbol->structSpec);
            typeModifier = &typeModifier->typeSymbol->structSpec->ptrtype;
        } else if (declarator->npointers >= 2 && builtinPtr2Ptr2TypeTable[typeModifier->type] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers -= 2;
            typeModifier = builtinPtr2Ptr2TypeTable[typeModifier->type];
        } else if (declarator->npointers >= 1 && builtinPtr2TypeTable[typeModifier->type] != NULL
                   && typeModifier->typedefSymbol == NULL) {
            assert(typeModifier->next == NULL); /* not a user defined type */
            declarator->npointers--;
            typeModifier = builtinPtr2TypeTable[typeModifier->type];
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
        r = newSymbolAsType(NULL, NULL, NO_POSITION, typeModifier);
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
                if (symbol->typeModifier == NULL)
                    symbol->typeModifier = ty->typeModifier;
            }
        }
    }
    return res;
}

static TypeModifier *createSimpleEnumType(Symbol *enumDefinition) {
    return newEnumTypeModifier(enumDefinition);
}

void initSymStructSpec(StructSpec *symStruct, Symbol *records) {
    *symStruct = (StructSpec){};
    symStruct->members = records;
}

TypeModifier *simpleStructOrUnionSpecifier(Id *typeName, Id *id, Usage usage) {
    log_debug("new struct %s", id->name);
    assert(typeName && typeName->symbol && typeName->symbol->type == TypeKeyword);
    assert(typeName->symbol->keyword == STRUCT
           ||  typeName->symbol->keyword == UNION);

    Type type = typeName->symbol->keyword == UNION? TypeUnion : TypeStruct;
    Symbol symbol = makeSymbol(id->name, type, id->position);
    symbol.type = type;
    symbol.storage = StorageDefault;

    Symbol *foundMemberP;
    if (!symbolTableIsMember(symbolTable, &symbol, NULL, &foundMemberP)
        || (isMemoryFromPreviousBlock(foundMemberP) && isDefinitionOrDeclarationUsage(usage)))
    {
        foundMemberP = stackMemoryAlloc(sizeof(Symbol));
        *foundMemberP = symbol;
        foundMemberP->structSpec = stackMemoryAlloc(sizeof(StructSpec));

        initSymStructSpec(foundMemberP->structSpec, NULL);

        TypeModifier *typeModifier = &foundMemberP->structSpec->type;
        /* Assumed to be Struct/Union/Enum? */
        initTypeModifierAsStructUnionOrEnum(typeModifier, type, foundMemberP, NULL, NULL);

        TypeModifier *ptrtypeModifier = &foundMemberP->structSpec->ptrtype;
        initTypeModifierAsPointer(ptrtypeModifier, &foundMemberP->structSpec->type);

        setGlobalFileDepNames(id->name, foundMemberP, MEMORY_XX);
        addSymbolToFrame(symbolTable, foundMemberP);
    }
    handleFoundSymbolReference(foundMemberP, id->position, usage, NO_FILE_NUMBER);
    return &foundMemberP->structSpec->type;
}

void setGlobalFileDepNames(char *iname, Symbol *symbol, MemoryClass memory) {
    char *mname, *fname;
    char tmp[MACRO_NAME_SIZE];
    int isMember, order, len, len2;

    if (iname == NULL)
        iname="";
    assert(symbol);
    if (options.exactPositionResolve) {
        FileItem *fileItem = getFileItemWithFileNumber(symbol->position.file);
        fname = simpleFileName(fileItem->name);
        sprintf(tmp, "%x-%s-%x-%x%c", hashFun(fileItem->name), fname, symbol->position.line,
                symbol->position.col, LINK_NAME_SEPARATOR);
    } else if (iname[0]==0) {
        Symbol *member;
        // anonymous enum/structure/union ...
        int fileNumber = symbol->position.file;
        symbol->name=iname;
        symbol->linkName=iname;
        order = 0;
        isMember = symbolTableIsMember(symbolTable, symbol, NULL, &member);
        while (isMember) {
            if (member->position.file==fileNumber)
                order++;
            isMember = symbolTableNextMember(symbol, &member);
        }
        fname = simpleFileName(getFileItemWithFileNumber(fileNumber)->name);
        sprintf(tmp, "%s%c%d%c", fname, FILE_PATH_SEPARATOR, order, LINK_NAME_SEPARATOR);
        /*&     // macros will be identified by name only?
          } else if (symbol->type == TypeMacro) {
          sprintf(tmp, "%x%c", symbol->position.file, LINK_NAME_SEPARATOR);
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
    assert(typeName->symbol->keyword == STRUCT
           ||  typeName->symbol->keyword == UNION
           );
    if (typeName->symbol->keyword == STRUCT) type = TypeStruct;
    else type = TypeUnion;

    symbol = newSymbol("", typeName->position);
    symbol->type = type;
    symbol->storage = StorageDefault;

    setGlobalFileDepNames("", symbol, MEMORY_XX);

    symbol->structSpec = stackMemoryAlloc(sizeof(StructSpec));

    /* This is a recurring pattern, create a struct and the pointer type to it*/
    initSymStructSpec(symbol->structSpec, /*.records=*/NULL);
    TypeModifier *stype = &symbol->structSpec->type;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(stype, /*.kind=*/type, /*.u.t=*/symbol,
                                        /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *sptrtype = &symbol->structSpec->ptrtype;
    initTypeModifierAsPointer(sptrtype, &symbol->structSpec->type);

    addSymbolToFrame(symbolTable, symbol);

    return &symbol->structSpec->type;
}

static char *string3ConcatInStackMem(char *str1, char *str2, char *str3) {
    int l1 = strlen(str1);
    int l2 = strlen(str2);
    int l3 = strlen(str3);
    char *s  = stackMemoryAlloc(l1 + l2 + l3 + 1);

    strcpy(s, str1);
    strcpy(s + l1, str2);
    strcpy(s + l1 + l2, str3);

    return s;
}

void specializeStructOrUnionDef(Symbol *symbol, Symbol *recordSymbol) {
    assert(symbol->type == TypeStruct || symbol->type == TypeUnion);
    assert(symbol->structSpec);
    if (symbol->structSpec->members!=NULL)
        return;

    symbol->structSpec->members = recordSymbol;
    addToFrame(setToNull, &(symbol->structSpec->members));
    for (Symbol *r=recordSymbol; r!=NULL; r=r->next) {
        if (r->name != NULL) {
            r->linkName = string3ConcatInStackMem(symbol->linkName, ".", r->name);
            handleFoundSymbolReference(r, r->position, UsageDefined, NO_FILE_NUMBER);
        }
    }
}

TypeModifier *simpleEnumSpecifier(Id *id, Usage usage) {
    Symbol symbol, *symbolP;

    symbol = makeSymbol(id->name, TypeEnum, id->position);
    if (!symbolTableIsMember(symbolTable, &symbol, NULL, &symbolP)
        || (isMemoryFromPreviousBlock(symbolP) && isDefinitionOrDeclarationUsage(usage))) {
        symbolP = stackMemoryAlloc(sizeof(Symbol));
        *symbolP = symbol;
        setGlobalFileDepNames(id->name, symbolP, MEMORY_XX);
        addSymbolToFrame(symbolTable, symbolP);
    }
    handleFoundSymbolReference(symbolP, id->position, usage, NO_FILE_NUMBER);
    return createSimpleEnumType(symbolP);
}

TypeModifier *createNewAnonymousEnum(SymbolList *enums) {
    Symbol *symbol;

    symbol = newSymbolAsEnum("", "", NO_POSITION, enums);
    symbol->type = TypeEnum;
    symbol->storage = StorageDefault;

    setGlobalFileDepNames("", symbol, MEMORY_XX);
    symbol->enums = enums;
    return createSimpleEnumType(symbol);
}

void appendPositionToList(PositionList **list, Position position) {
    PositionList *ppl;
    ppl = newPositionList(position, NULL);
    LIST_APPEND(PositionList, (*list), ppl);
}

void setParamPositionForFunctionWithoutParams(Position lparPosition) {
    parameterBeginPosition = lparPosition;
    parameterEndPosition = lparPosition;
}

static void setParamPositionForParameter0(Position lparPosition) {
    parameterBeginPosition = lparPosition;
    parameterEndPosition = lparPosition;
}

void setParamPositionForParameterBeyondRange(Position rparPosition) {
    parameterBeginPosition = rparPosition;
    parameterEndPosition = rparPosition;
}

static void handleParameterPositions(Position lparPosition, PositionList *commaPositions,
                                     Position rparPosition, bool hasParameters, bool isVoid) {
    Position     position1, position2;
    PositionList *list;

    /* Sets the following global variables as "answer":
       - parameterListIsVoid
       - parameterCount
       - parameterBeginPosition
       - parameterEndPosition
    */

    parameterListIsVoid = isVoid;
    parameterCount = 0;

    if (!hasParameters) {
        setParamPositionForFunctionWithoutParams(lparPosition);
        return;
    }

    if (!isVoid) {
        LIST_LEN(parameterCount, PositionList, commaPositions);
        parameterCount++;
    }

    int argn = parsingConfig.targetParameterIndex;
    assert(argn > 0);           /* TODO: WTF is Parameter0 and when is it used? */
    if (argn == 0) {
        setParamPositionForParameter0(lparPosition);
    } else {
        list = commaPositions;
        position1 = lparPosition;
        if (list != NULL)
            position2 = list->position;
        else
            position2 = rparPosition;
        int i;
        for (i=2; list != NULL && i <= argn; list = list->next, i++) {
            position1 = list->position;
            if (list->next != NULL)
                position2 = list->next->position;
            else
                position2 = rparPosition;
        }
        if (list == NULL && i <= argn) {
            setParamPositionForParameterBeyondRange(rparPosition);
        } else {
            parameterBeginPosition = position1;
            parameterEndPosition   = position2;
        }
    }
}

Symbol *createEmptyField(void) {
    TypeModifier *typeModifier;

    typeModifier = newSimpleTypeModifier(TypeAnonymousField);
    return newSymbolAsType("", "", NO_POSITION, typeModifier);
}

void handleDeclaratorParamPositions(Symbol *symbol, Position lparPosition,
                                    PositionList *commaPositions, Position rparPositions,
                                    bool hasParameters, bool isVoid) {
    if (options.mode != ServerMode)
        return;
    if (parsingConfig.operation != PARSE_TO_TRACK_PARAMETERS)
        return;
    if (positionsAreNotEqual(symbol->position, parsingConfig.positionOfSelectedReference))
        return;
    handleParameterPositions(lparPosition, commaPositions, rparPositions, hasParameters, isVoid);
}

void handleInvocationParameterPositions(Reference *reference, Position lparPosition,
                                        PositionList *commaPositions, Position rparPosition,
                                        bool hasParameters) {
    if (options.mode != ServerMode)
        return;
    if (parsingConfig.operation != PARSE_TO_TRACK_PARAMETERS)
        return;
    if (reference==NULL || positionsAreNotEqual(reference->position, parsingConfig.positionOfSelectedReference))
        return;
    handleParameterPositions(lparPosition, commaPositions, rparPosition, hasParameters, false);
}
