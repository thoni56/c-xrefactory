#include "yylex.h"
#define _MACROARGUMENTTABLE_
#include "macroargumenttable.h"

#include "hash.h"
#include "commons.h"

#define HASH_FUN(elementP) hashFun(elementP->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

MacroArgumentTable macroArgumentTable;

void allocateMacroArgumentTable(int count) {
    PPM_ALLOCC(macroArgumentTable.tab, MAX_MACRO_ARGS, MacroArgumentTableElement *);
    macroArgumentTableNoAllocInit(&macroArgumentTable, MAX_MACRO_ARGS);
}

MacroArgumentTableElement *getMacroArgument(int index) {
    return macroArgumentTable.tab[index];
}

int addMacroArgument(MacroArgumentTableElement *element) {
    return macroArgumentTableAdd(&macroArgumentTable, element);
}

bool isMemberInMacroArguments(MacroArgumentTableElement *element, int *foundIndex) {
    return macroArgumentTableIsMember(&macroArgumentTable, element, foundIndex);
}

void mapOverMacroArgumentsWithPointer(void (*fun)(MacroArgumentTableElement *element, void *pointer),
                                      void *pointer) {
    macroArgumentTableMapWithPointer(&macroArgumentTable, fun, pointer);
}
