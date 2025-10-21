#ifndef MACROARGUMENTTABLE_H_INCLUDED
#define MACROARGUMENTTABLE_H_INCLUDED

/* Macro argument table - instance of hashtable */

#include "yylex.h"

extern void allocateMacroArgumentTable(int count);
extern void resetMacroArgumentTable(void);
extern MacroArgumentTableElement makeMacroArgumentTableElement(char *name, char *linkName, int order);
extern MacroArgumentTableElement *getMacroArgument(int index);
extern int addMacroArgument(MacroArgumentTableElement *element);
extern bool isMemberInMacroArguments(MacroArgumentTableElement *element, int *foundIndex);
extern void mapOverMacroArgumentsWithPointer(void (*fun)(MacroArgumentTableElement *element,
                                                         void *pointer),
                                             void *pointer);

#endif
