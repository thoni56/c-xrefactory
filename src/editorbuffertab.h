#ifndef EDITORBUFFERTAB_H_INCLUDED
#define EDITORBUFFERTAB_H_INCLUDED

#include "editor.h"

#define HASH_TAB_NAME editorBufferTab
#define HASH_TAB_TYPE EditorBufferTab
#define HASH_ELEM_TYPE EditorBufferList

#include "hashlist.th"

#ifndef IN_EDITORBUFFERTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

#define EDITOR_BUFF_TAB_SIZE 100

extern EditorBufferTab editorBufferTable;

extern void initEditorBufferTable(void);
extern int addEditorBuffer(EditorBufferList *bufferList);

#endif
