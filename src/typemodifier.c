#include "typemodifier.h"

#include <stdio.h>


/* For typeModifiers we need to cater for two memory allocations (XX &
   CF) as well as the ancient FILL semantics... */

/* The most common is to allocate in XX_memory == StackMemAlloc() */
static S_typeModifier *newTypeModifier(Type kind, Symbol *typedefSymbol, S_typeModifier *next) {
    S_typeModifier *typeModifier = StackMemAlloc(S_typeModifier);

    typeModifier->kind = kind;
    typeModifier->typedefSymbol = typedefSymbol;
    typeModifier->next = next;

    return typeModifier;
}

S_typeModifier *newFunctionTypeModifier(Symbol *args, Symbol **overLoadList, Symbol *typedefSymbol, S_typeModifier *next) {
    S_typeModifier *typeModifier = newTypeModifier(TypeFunction, NULL, NULL);

    typeModifier->u.f.args = args;
    typeModifier->u.f.thisFunList = overLoadList;

    return typeModifier;
}
