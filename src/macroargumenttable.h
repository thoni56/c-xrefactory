#ifndef MACROARGUMENTTABLE_H_INCLUDED
#define MACROARGUMENTTABLE_H_INCLUDED

/* Macro argument table - instance of hashtable */

#include "yylex.h"

#define HASH_TAB_NAME macroArgumentTable
#define HASH_TAB_TYPE MacroArgumentTable
#define HASH_ELEM_TYPE MacroArgumentTableElement

#include "hashtab.th"

extern MacroArgumentTable macroArgumentTable;

#ifndef _MACROARGUMENTTABLE_
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void allocateMacroArgumentTable(int count);
extern MacroArgumentTableElement *getMacroArgument(int index);
extern int addMacroArgument(MacroArgumentTableElement *element);
extern bool isMemberInMacroArguments(MacroArgumentTableElement *element, int *foundIndex);
extern void mapOverMacroArgumentsWithPointer(void (*fun)(MacroArgumentTableElement *element,
                                                         void *pointer),
                                             void *pointer);

#endif
