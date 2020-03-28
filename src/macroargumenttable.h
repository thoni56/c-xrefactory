#ifndef _MACROARGUMENTTABLE_H_
#define _MACROARGUMENTTABLE_H_

/* Macro argument table - instance of hashtable */

#include "yylex.h"

#define HASH_TAB_NAME macroArgumentTable
#define HASH_ELEM_TYPE S_macroArgumentTableElement

#include "hashtab.th"

extern S_macroArgumentTable s_macroArgumentTable;

#ifndef _MACROARGUMENTTABLE_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
