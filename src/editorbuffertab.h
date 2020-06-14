#ifndef _EDITORBUFFERTAB_H_
#define _EDITORBUFFERTAB_H_

#include "editor.h"

#define HASH_TAB_NAME editorBufferTab
#define HASH_TAB_TYPE EditorBufferTab
#define HASH_ELEM_TYPE S_editorBufferList

#include "hashlist-new.th"

#ifndef _EDITORBUFFERTAB_
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

#define EDITOR_BUFF_TAB_SIZE 100

extern S_editorBufferList *s_staticEditorBufferTabTab[];
extern EditorBufferTab s_editorBufferTab;

#endif
