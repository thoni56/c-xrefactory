#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"


#define HASH_FUN(element) hashFun(element->buffer->fileName)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->buffer->fileName,e2->buffer->fileName)==0)

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

bool editorBufferIsMember(EditorBufferList *elementP, int *positionP, EditorBufferList **foundMemberP) {
    return editorBufferTabIsMember(&editorBufferTable, elementP, positionP, foundMemberP);
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
