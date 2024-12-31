#define IN_EDITORBUFFERTAB_C
#include "editorbuffertab.h"


static unsigned editorBufferHashFun(char *fileName);

#define HASH_FUN(element) editorBufferHashFun(element->buffer->fileName)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->buffer->fileName,e2->buffer->fileName)==0)

#include "hashlist.tc"

#define EDITOR_BUFFER_TABLE_SIZE 100

/* Make it possible to inject a mocked hashFun() */
static unsigned (*mockedHashFun)(char *fileName) = NULL;
static unsigned editorBufferHashFun(char *fileName) {
    if (mockedHashFun == NULL)
        return hashFun(fileName);
    else
        return mockedHashFun(fileName);
}

void injectHashFun(unsigned (fun)(char *fileName)) {
    mockedHashFun = fun;
}

static EditorBufferList *editorBufferTablesInit[EDITOR_BUFFER_TABLE_SIZE];
protected EditorBufferTab editorBufferTable;


void initEditorBufferTable() {
    editorBufferTable.tab = editorBufferTablesInit;
    editorBufferTabNoAllocInit(&editorBufferTable, EDITOR_BUFFER_TABLE_SIZE);
}

int addEditorBuffer(EditorBufferList *bufferListElement) {
    return editorBufferTabAdd(&editorBufferTable, bufferListElement);
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

void editorBufferTableEntryPop(int index) {
    setEditorBuffer(index, editorBufferTable.tab[index]->next);
}
