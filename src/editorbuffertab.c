#include "editor.h"
#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"

#include "hash.h"

#define HASH_FUN(elemp) hashFun(elemp->buffer->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->buffer->name,e2->buffer->name)==0)

#include "hashlist.tc"


static EditorBufferList *editorBufferTablesInit[EDITOR_BUFF_TAB_SIZE];
static EditorBufferTab editorBufferTable;

void initEditorBufferTable() {
    editorBufferTable.tab = editorBufferTablesInit;
    editorBufferTabNoAllocInit(&editorBufferTable, EDITOR_BUFF_TAB_SIZE);
}

int addEditorBuffer(EditorBufferList *bufferList) {
    return editorBufferTabAdd(&editorBufferTable, bufferList);
}

bool editorBufferIsMember(EditorBufferList *elementP, int *positionP, EditorBufferList **originP) {
    return editorBufferTabIsMember(&editorBufferTable, elementP, positionP, originP);
}

bool deleteEditorBuffer(EditorBufferList *elementP) {
    return editorBufferTabDeleteExact(&editorBufferTable, elementP);
}

EditorBufferList *getEditorBuffer(int index) {
    return editorBufferTable.tab[index];
}

int getNextExistingEditorBufferIndex(int index) {
    for (int i=index; i < editorBufferTable.size; i++)
        if (editorBufferTable.tab[i] != NULL)
            return i;
    return -1;
}

void setEditorBuffer(int index, EditorBufferList *elementP) {
    editorBufferTable.tab[index] = elementP;
}

void clearEditorBuffer(int index) {
    editorBufferTable.tab[index] = NULL;
}
