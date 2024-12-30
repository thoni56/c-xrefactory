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


extern void initEditorBufferTable(void);
extern int  addEditorBuffer(EditorBufferList *bufferList);
extern bool editorBufferIsMember(EditorBufferList *elementP, int *positionP, EditorBufferList **originP);
extern bool deleteEditorBuffer(EditorBufferList *element);
extern int  getNextExistingEditorBufferIndex(int index);
extern EditorBufferList *getEditorBuffer(int index);
extern void              setEditorBuffer(int index, EditorBufferList *buffer);
extern void editorBufferTableEntryPop(int index);
extern void clearEditorBuffer(int index);

#endif
