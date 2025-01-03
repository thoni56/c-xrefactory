#ifndef EDITORBUFFERTABLE_H_INCLUDED
#define EDITORBUFFERTABLE_H_INCLUDED

#include "editorbuffer.h"

#define HASH_TAB_NAME editorBufferTable
#define HASH_TAB_TYPE EditorBufferTable
#define HASH_ELEM_TYPE EditorBufferList

#include "hashlist.th"

#ifndef IN_EDITORBUFFERTABLE_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

/* For testing */
extern void injectHashFun(unsigned(fun)(char *fileName));

extern void initEditorBufferTable(void);
extern int  addEditorBuffer(EditorBufferList *bufferList);
extern bool editorBufferIsMember(EditorBufferList *elementP, int *positionP, EditorBufferList **originP);
extern bool deleteEditorBuffer(EditorBufferList *element);
extern int  getNextExistingEditorBufferIndex(int index);
extern EditorBufferList *getEditorBufferListElementAt(int index);
extern void              setEditorBuffer(int index, EditorBufferList *buffer);
extern void clearEditorBuffer(int index);

/* Register and deregister will hide the lists that the hashlist uses */
extern int registerEditorBuffer(EditorBuffer *buffer);
extern EditorBuffer *deregisterEditorBuffer(char *fileName);
extern EditorBuffer *getEditorBufferForFile(char *fileName);

#endif
