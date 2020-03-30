#include "typemodifier.h"

#include <stdio.h>
#include <string.h>


static void fillTypeModifier(S_typeModifier *typeModifier, Type kind, Symbol *typedefSymbol, S_typeModifier *next) {
    memset(typeModifier, 0, sizeof(*typeModifier));
    typeModifier->kind = kind;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;
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
    S_typeModifier *typeModifier = newTypeModifier(TypeFunction, NULL, NULL);

    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overLoadList;

    return typeModifier;
}

S_typeModifier *newArrayTypeModifier(void) {
    return newTypeModifier(TypeArray, NULL, NULL);
}

S_typeModifier *newStructTypeModifier(void) {
    S_typeModifier *typeModifier = newTypeModifier(TypeStruct, NULL, NULL);
    typeModifier->u.t = NULL;
    return typeModifier;
}
