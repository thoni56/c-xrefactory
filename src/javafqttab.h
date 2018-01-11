#ifndef _JAVAFQTTAB_H_
#define _JAVAFQTTAB_H_

#include "proto.h"

#define HASH_TAB_NAME javaFqtTab
#define HASH_ELEM_TYPE S_symbolList

#include "hashlist.th"

#ifndef _JAVAFQTTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
