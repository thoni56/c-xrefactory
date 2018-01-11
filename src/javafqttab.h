#ifndef _JAVAFQTTAB_H_
#define _JAVAFQTTAB_H_

#include "proto.h"

#define HASH_TAB_TYPE struct javaFqtTab
#define HASH_ELEM_TYPE S_symbolList
#define HASH_FUN_PREFIX javaFqtTab

#include "hashlist.th"

#ifndef _JAVAFQTTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
