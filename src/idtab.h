#ifndef _IDTAB_H_
#define _IDTAB_H_

#include "proto.h"

#define HASH_TAB_TYPE struct idTab
#define HASH_ELEM_TYPE S_fileItem
#define HASH_FUN_PREFIX idTab

#include "hashtab.th"

#ifndef _IDTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#endif
