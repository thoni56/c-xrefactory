#include "typemodifiers.h"

/* For typeModifiers we need to cater for two memory allocations (XX &
   CF) as well as the ancient FILL semantics... */

/* The most common is to allocate in XX_memory == StackMemAlloc() */
static S_typeModifiers *newTypeModifiers(enum type kind) {
    S_typeModifiers *typeModifiers = StackMemAlloc(S_typeModifiers);

    typeModifiers->kind = kind;
    typeModifiers->typedefSymbol = NULL;
    typeModifiers->next = NULL;
}

S_typeModifiers *newFunctionTypeModifier(void) {
    S_typeModifiers *typeModifiers = newTypeModifiers(TypeFunction);
    typeModifiers->u.f.args == NULL;
    typeModifiers->u.f.thisFunList = NULL;
}
