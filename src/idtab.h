#ifndef _IDTAB_H_
#define _IDTAB_H_

#include "proto.h"

#define HASH_TAB_NAME idTab
#define HASH_ELEM_TYPE S_fileItem

#include "hashtab.th"

#ifndef _IDTAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#endif
