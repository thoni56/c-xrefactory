#include "typemodifier.h"

#include <stdio.h>
#include <string.h>
#include "commons.h"


static void fillTypeModifier(S_typeModifier *typeModifier, Type kind, Symbol *typedefSymbol, S_typeModifier *next) {
    memset(typeModifier, 0, sizeof(*typeModifier));
    typeModifier->kind = kind;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifier(S_typeModifier *typeModifier, Type kind) {
    fillTypeModifier(typeModifier, kind, NULL, NULL);
}

void initTypeModifierAsStructUnionOrEnum(S_typeModifier *typeModifier, Type kind, Symbol *symbol, Symbol *typedefSymbol, S_typeModifier *next) {
    assert(kind == TypeStruct || kind == TypeUnion || kind == TypeEnum);
    typeModifier->kind = kind;
    typeModifier->u.t = symbol;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsMethod(S_typeModifier *typeModifier, char *signature, SymbolList *exceptions, Symbol *typedefSymbol, S_typeModifier *next) {
    typeModifier->kind = TypeFunction;
    typeModifier->u.m.signature = signature;
    typeModifier->u.m.exceptions = exceptions;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsFunction(S_typeModifier *typeModifier, Symbol *args, Symbol **overloadFunctionList, Symbol *typedefSymbol, S_typeModifier *next) {
    typeModifier->kind = TypeFunction;
    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overloadFunctionList;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifierAsPointer(S_typeModifier *typeModifier, S_typeModifier *next) {
    fillTypeModifier(typeModifier, TypePointer, NULL, next);
}

void initTypeModifierAsArray(S_typeModifier *typeModifier,Symbol *typedefSymbol, S_typeModifier *next) {
    fillTypeModifier(typeModifier, TypeArray, typedefSymbol, next);
}


void initFunctionTypeModifier(struct functionTypeModifier *modifier, Symbol *args) {
    modifier->args = args;
    modifier->thisFunList = NULL;
}


/* For typeModifiers we need to cater for two memory allocations (XX &
   CF) as well as the ancient FILL semantics... */

/* The most common is to allocate in XX_memory == StackMemAlloc() */
S_typeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, S_typeModifier *next) {
    S_typeModifier *typeModifier = StackMemAlloc(S_typeModifier);

    fillTypeModifier(typeModifier, kind, typedefSymbol, next);

    return typeModifier;
}

S_typeModifier *newFunctionTypeModifier(Symbol *args, Symbol **overLoadList, Symbol *typedefSymbol, S_typeModifier *next) {
    S_typeModifier *typeModifier = newTypeModifier(TypeFunction, typedefSymbol, next);

    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overLoadList;

    return typeModifier;
}

S_typeModifier *newSimpleTypeModifier(Type kind) {
    return newTypeModifier(kind, NULL, NULL);
}

S_typeModifier *newPointerTypeModifier(S_typeModifier *next) {
    return newTypeModifier(TypePointer, NULL, next);
}

S_typeModifier *newArrayTypeModifier(void) {
    return newTypeModifier(TypeArray, NULL, NULL);
}

S_typeModifier *newStructTypeModifier(Symbol *symbol) {
    S_typeModifier *typeModifier = newTypeModifier(TypeStruct, NULL, NULL);
    typeModifier->u.t = symbol;
    return typeModifier;
}

S_typeModifier *newEnumTypeModifier(Symbol *symbol) {
    S_typeModifier *typeModifier = newTypeModifier(TypeEnum, NULL, NULL);
    typeModifier->u.t = symbol;
    return typeModifier;
}

S_typeModifierList *newTypeModifierList(S_typeModifier *d) {
    S_typeModifierList *list;
    XX_ALLOC(list, S_typeModifierList);
    list->d = d;
    list->next = NULL;
    return list;
}

S_typeModifier *prependTypeModifierWith(S_typeModifier *this, Type kind) {
    return newTypeModifier(kind, NULL, this);
}
