#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"

#include "hash.h"

#define HASH_FUN(elemp) hashFun(elemp->buffer->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->buffer->name,e2->buffer->name)==0)

#include "hashlist.tc"

EditorBufferTab editorBufferTable;

static EditorBufferList *editorBufferTablesInit[EDITOR_BUFF_TAB_SIZE];

void initEditorBufferTable() {
    editorBufferTable.tab = editorBufferTablesInit;
    editorBufferTabNoAllocInit(&editorBufferTable, EDITOR_BUFF_TAB_SIZE);
}
