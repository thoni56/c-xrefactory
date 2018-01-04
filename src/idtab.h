#ifndef IDTAB_H
#define IDTAB_H

#include "proto.h"

#define HASH_TAB_TYPE struct idTab
#define HASH_ELEM_TYPE S_fileItem
#define HASH_FUN_PREFIX idTab

#include "hashtab.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
