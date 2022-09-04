#include "macroargumenttable.h"

/* Define the hashtab: */
#define HASH_TAB_NAME macroArgumentTable
#define HASH_TAB_TYPE MacroArgumentTable
#define HASH_ELEM_TYPE MacroArgumentTableElement

#include "hashtab.th"

/* Define hashtab functions: */
#include "hash.h"
#include "commons.h"

#define HASH_FUN(elementP) hashFun(elementP->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

static MacroArgumentTable macroArgumentTable;


void allocateMacroArgumentTable(int count) {
    PPM_ALLOCC(macroArgumentTable.tab, MAX_MACRO_ARGS, MacroArgumentTableElement *);
    macroArgumentTableNoAllocInit(&macroArgumentTable, MAX_MACRO_ARGS);
}

void resetMacroArgumentTable(void) {
    assert(macroArgumentTable.size > 0);
    macroArgumentTableNoAllocInit(&macroArgumentTable, macroArgumentTable.size);
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
