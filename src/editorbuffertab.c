#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->f->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->f->name,e2->f->name)==0)

#include "hashlist.tc"

EditorBufferList *s_staticEditorBufferTabTab[EDITOR_BUFF_TAB_SIZE];
EditorBufferTab s_editorBufferTab;
