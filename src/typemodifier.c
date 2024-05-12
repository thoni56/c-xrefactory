#include "typemodifier.h"

#include <stdio.h>
#include <string.h>
#include "commons.h"
#include "stackmemory.h"


static void fillTypeModifier(TypeModifier *typeModifier, Type type, Symbol *typedefSymbol, TypeModifier *next) {
    memset(typeModifier, 0, sizeof(*typeModifier));
    typeModifier->type = type;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
}

void initTypeModifier(TypeModifier *typeModifier, Type type) {
    fillTypeModifier(typeModifier, type, NULL, NULL);
}

void initTypeModifierAsStructUnionOrEnum(TypeModifier *typeModifier, Type type, Symbol *symbol, Symbol *typedefSymbol, TypeModifier *next) {
    assert(type == TypeStruct || type == TypeUnion || type == TypeEnum);
    typeModifier->type = type;
    typeModifier->u.t = symbol;
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
}


/* For typeModifiers we need to cater for two memory allocations (XX &
   CF) as well as the ancient FILL semantics... */

/* The most common is to allocate in XX_memory == stackMemAlloc(sizeof()) so this is what we do here */
TypeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, TypeModifier *next) {
    TypeModifier *typeModifier = stackMemoryAlloc(sizeof(TypeModifier));

    fillTypeModifier(typeModifier, kind, typedefSymbol, next);

    return typeModifier;
}

TypeModifier *newFunctionTypeModifier(Symbol *args, Symbol *typedefSymbol, TypeModifier *next) {
    TypeModifier *typeModifier = newTypeModifier(TypeFunction, typedefSymbol, next);

    typeModifier->u.f.args = args;

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

TypeModifier *prependTypeModifierWith(TypeModifier *thisModifier, Type kind) {
    return newTypeModifier(kind, NULL, thisModifier);
}
