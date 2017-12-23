#ifndef JAVAFQTTAB_H
#define JAVAFQTTAB_H

#define HASH_TAB_TYPE struct javaFqtTab
#define HASH_ELEM_TYPE S_symbolList
#define HASH_FUN_PREFIX javaFqtTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
