#define _EDITORBUFFERTAB_
#include "editorbuffertab.h"

#include "hash.h"

#include "misc.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->f->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->f->name,e2->f->name)==0)

#include "hashlist.tc"

S_editorBufferList *s_staticEditorBufferTabTab[EDITOR_BUFF_TAB_SIZE];
S_editorBufferTab s_editorBufferTab;
