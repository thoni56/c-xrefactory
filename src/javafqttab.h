#ifndef _JAVAFQTTAB_H_
#define _JAVAFQTTAB_H_

#include "proto.h"
#include "symbol.h"

#define HASH_TAB_NAME javaFqtTable
#define HASH_TAB_TYPE JavaFqtTable
#define HASH_ELEM_TYPE SymbolList

#include "hashlist-new.th"

#ifndef _IN_JAVAFQTTAB_C_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

extern JavaFqtTable javaFqtTable;

#endif
