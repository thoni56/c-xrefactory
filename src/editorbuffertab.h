#ifndef EDITORBUFFERTAB_H
#define EDITORBUFFERTAB_H

#define HASH_TAB_TYPE struct editorBufferTab
#define HASH_ELEM_TYPE S_editorBufferList
#define HASH_FUN_PREFIX editorBufferTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#endif
