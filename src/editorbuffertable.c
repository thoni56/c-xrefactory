#include "editorbuffer.h"
#define IN_EDITORBUFFERTABLE_C
#include "editorbuffertable.h"

#include <stdlib.h>

#include "head.h"


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
protected EditorBufferTable editorBufferTable;


void initEditorBufferTable() {
    editorBufferTable.tab = editorBufferTablesInit;
    editorBufferTableNoAllocInit(&editorBufferTable, EDITOR_BUFFER_TABLE_SIZE);
}

int addEditorBuffer(EditorBufferList *bufferListElement) {
    return editorBufferTableAdd(&editorBufferTable, bufferListElement);
}

bool editorBufferIsMember(EditorBufferList *elementP, int *positionP, EditorBufferList **foundMemberP) {
    return editorBufferTableIsMember(&editorBufferTable, elementP, positionP, foundMemberP);
}

bool deleteEditorBuffer(EditorBufferList *elementP) {
    return editorBufferTableDeleteExact(&editorBufferTable, elementP);
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

/**
 * @brief Registers an EditorBuffer in the editor buffer table
 *
 * This function allocates the necessary EditorBufferList element so
 * that the caller does not have to bother with that.
 *
 * @param buffer - buffer owned by the caller to register.
 * @return the index in the editor buffer table. NB. There might be more at the same index.
 */
int registerEditorBuffer(EditorBuffer *buffer) {
    EditorBufferList *list = malloc(sizeof(EditorBufferList));
    *list = (EditorBufferList){.buffer = buffer, .next = NULL};
    return addEditorBuffer(list); /* TODO Handles existing, and hash collisions? */
}

/**
 * @brief Deregisters a buffer from the editor buffer table
 *
 * Frees the necessary list element but returns the item itself to the
 * caller to free.
 *
 * @param fileName - the name of the file it was loaded from.
 * @returns the EditorBuffer that was deregistered or NULL if not found.
 */
EditorBuffer *deregisterEditorBuffer(char *fileName) {
    EditorBuffer *buffer = malloc(sizeof(EditorBuffer));
    *buffer = (EditorBuffer){.fileName = fileName};
    EditorBufferList *list = malloc(sizeof(EditorBufferList));
    *list = (EditorBufferList){.buffer = buffer, .next = NULL};

    int foundIndex;
    if (!editorBufferIsMember(list, &foundIndex, NULL))
        return NULL;

    EditorBuffer *found_buffer = editorBufferTable.tab[foundIndex]->buffer;
    if (strcmp(found_buffer->fileName, fileName) == 0) {
        EditorBufferList *list_element = editorBufferTable.tab[foundIndex];
        editorBufferTable.tab[foundIndex] = list_element->next;
        free(list_element);
        return found_buffer;
    } else {
        /* Assume it is the second in the list */
        EditorBuffer *found_buffer = editorBufferTable.tab[foundIndex]->next->buffer;
        EditorBufferList *list_element = editorBufferTable.tab[foundIndex]->next;
        editorBufferTable.tab[foundIndex]->next = list_element->next;
        free(list_element);
        return found_buffer;
    }
}
