#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->buffer->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->buffer->name,e2->buffer->name)==0)

#include "hashlist.tc"

EditorBufferList *s_staticEditorBufferTabTab[EDITOR_BUFF_TAB_SIZE];
EditorBufferTab s_editorBufferTab;
