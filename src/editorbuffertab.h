#ifndef _EDITORBUFFERTAB_H_
#define _EDITORBUFFERTAB_H_

#include "proto.h"

#define HASH_TAB_TYPE struct editorBufferTab
#define HASH_ELEM_TYPE S_editorBufferList
#define HASH_FUN_PREFIX editorBufferTab

#include "hashlist.th"

#ifndef _EDITORBUFFERTAB_
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#endif

#define EDITOR_BUFF_TAB_SIZE 100

extern S_editorBufferList *s_staticEditorBufferTabTab[];
extern S_editorBufferTab s_editorBufferTab;

#endif
