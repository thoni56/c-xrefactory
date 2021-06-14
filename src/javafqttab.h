#ifndef JAVAFQTTAB_H_INCLUDED
#define JAVAFQTTAB_H_INCLUDED

#include "proto.h"
#include "symbol.h"

#define HASH_TAB_NAME javaFqtTable
#define HASH_TAB_TYPE JavaFqtTable
#define HASH_ELEM_TYPE SymbolList

#include "hashlist.th"

#ifndef IN_JAVAFQTTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern JavaFqtTable javaFqtTable;

#endif
