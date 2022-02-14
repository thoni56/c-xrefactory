#include "typemodifier.h"

#include <stdio.h>
#include <string.h>
#include "commons.h"


static void fillTypeModifier(TypeModifier *typeModifier, Type kind, Symbol *typedefSymbol, TypeModifier *next) {
    memset(typeModifier, 0, sizeof(*typeModifier));
    typeModifier->kind = kind;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifier(TypeModifier *typeModifier, Type kind) {
    fillTypeModifier(typeModifier, kind, NULL, NULL);
}

void initTypeModifierAsStructUnionOrEnum(TypeModifier *typeModifier, Type kind, Symbol *symbol, Symbol *typedefSymbol, TypeModifier *next) {
    assert(kind == TypeStruct || kind == TypeUnion || kind == TypeEnum);
    typeModifier->kind = kind;
    typeModifier->u.t = symbol;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsMethod(TypeModifier *typeModifier, char *signature, SymbolList *exceptions, Symbol *typedefSymbol, TypeModifier *next) {
    typeModifier->kind = TypeFunction;
    typeModifier->u.m.signature = signature;
    typeModifier->u.m.exceptions = exceptions;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsFunction(TypeModifier *typeModifier, Symbol *args, Symbol **overloadFunctionList, Symbol *typedefSymbol, TypeModifier *next) {
    typeModifier->kind = TypeFunction;
    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overloadFunctionList;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsPointer(TypeModifier *typeModifier, TypeModifier *next) {
    fillTypeModifier(typeModifier, TypePointer, NULL, next);
}

void initTypeModifierAsArray(TypeModifier *typeModifier,Symbol *typedefSymbol, TypeModifier *next) {
    fillTypeModifier(typeModifier, TypeArray, typedefSymbol, next);
}


void initFunctionTypeModifier(struct functionTypeModifier *modifier, Symbol *args) {
    modifier->args = args;
    modifier->thisFunList = NULL;
}


/* For typeModifiers we need to cater for two memory allocations (XX &
   CF) as well as the ancient FILL semantics... */

/* The most common is to allocate in XX_memory == StackMemAlloc() */
TypeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, TypeModifier *next) {
    TypeModifier *typeModifier = StackMemoryAlloc(TypeModifier);

    fillTypeModifier(typeModifier, kind, typedefSymbol, next);

    return typeModifier;
}

TypeModifier *newFunctionTypeModifier(Symbol *args, Symbol **overLoadList, Symbol *typedefSymbol, TypeModifier *next) {
    TypeModifier *typeModifier = newTypeModifier(TypeFunction, typedefSymbol, next);

    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overLoadList;

    return typeModifier;
}

TypeModifier *newSimpleTypeModifier(Type kind) {
    return newTypeModifier(kind, NULL, NULL);
}

TypeModifier *newPointerTypeModifier(TypeModifier *next) {
    return newTypeModifier(TypePointer, NULL, next);
}

TypeModifier *newArrayTypeModifier(void) {
    return newTypeModifier(TypeArray, NULL, NULL);
}

TypeModifier *newStructTypeModifier(Symbol *symbol) {
    TypeModifier *typeModifier = newTypeModifier(TypeStruct, NULL, NULL);
    typeModifier->u.t = symbol;
    return typeModifier;
}

TypeModifier *newEnumTypeModifier(Symbol *symbol) {
    TypeModifier *typeModifier = newTypeModifier(TypeEnum, NULL, NULL);
    typeModifier->u.t = symbol;
    return typeModifier;
}

S_typeModifierList *newTypeModifierList(TypeModifier *d) {
    S_typeModifierList *list;
    list = StackMemoryAlloc(S_typeModifierList);
    list->d = d;
    list->next = NULL;
    return list;
}

TypeModifier *prependTypeModifierWith(TypeModifier *thisModifier, Type kind) {
    return newTypeModifier(kind, NULL, thisModifier);
}
