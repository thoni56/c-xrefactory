#include "editorbuffer.h"

#include "commons.h"
#include "editor.h"
#include "editorbuffertab.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "memory.h"


static void checkForMagicMarker(EditorBufferAllocationData *allocation) {
    assert(allocation->allocatedBlock[allocation->allocatedSize] == 0x3b);
}


void freeEditorBuffer(EditorBufferList *list) {
    if (list == NULL)
        return;
    log_trace("freeing buffer %s==%s", list->buffer->name, list->buffer->fileName);
    if (list->buffer->fileName != list->buffer->name) {
        editorFree(list->buffer->fileName, strlen(list->buffer->fileName)+1);
    }
    editorFree(list->buffer->name, strlen(list->buffer->name)+1);
    for (EditorMarker *marker=list->buffer->markers; marker!=NULL;) {
        EditorMarker *next = marker->next;
        editorFree(marker, sizeof(EditorMarker));
        marker = next;
    }
    if (list->buffer->textLoaded) {
        log_trace("freeing %d of size %d", list->buffer->allocation.allocatedBlock,
                  list->buffer->allocation.allocatedSize);
        checkForMagicMarker(&list->buffer->allocation);
        freeTextSpace(list->buffer->allocation.allocatedBlock,
                      list->buffer->allocation.allocatedIndex);
    }
    editorFree(list->buffer, sizeof(EditorBuffer));
    editorFree(list, sizeof(EditorBufferList));
}

EditorBuffer *createNewEditorBuffer(char *name, char *fileName, time_t modificationTime,
                                    size_t size) {
    char *allocatedName, *normalizedName, *afname, *normalizedFileName;
    EditorBuffer *buffer;
    EditorBufferList *bufferList;

    normalizedName = normalizeFileName(name, cwd);
    allocatedName = editorAlloc(strlen(normalizedName)+1);
    strcpy(allocatedName, normalizedName);
    normalizedFileName = normalizeFileName(fileName, cwd);
    if (strcmp(normalizedFileName, allocatedName)==0) {
        afname = allocatedName;
    } else {
        afname = editorAlloc(strlen(normalizedFileName)+1);
        strcpy(afname, normalizedFileName);
    }
    buffer = editorAlloc(sizeof(EditorBuffer));
    fillEmptyEditorBuffer(buffer, allocatedName, 0, afname);
    buffer->modificationTime = modificationTime;
    buffer->size = size;

    bufferList = editorAlloc(sizeof(EditorBufferList));
    *bufferList = (EditorBufferList){.buffer = buffer, .next = NULL};
    log_trace("creating buffer '%s' for '%s'", buffer->name, buffer->fileName);

    addEditorBuffer(bufferList);

    // set ftnum at the end, because, addfiletabitem calls back the statb
    // from editor, so be tip-top at this moment!
    buffer->fileNumber = addFileNameToFileTable(allocatedName);

    return buffer;
}

EditorBuffer *findEditorBufferForFile(char *name) {
    EditorBuffer *editorBuffer = getOpenedAndLoadedEditorBuffer(name);

    if (editorBuffer==NULL) {
        editorBuffer = getOpenedEditorBuffer(name);
        if (editorBuffer == NULL) {
            if (fileExists(name) && !isDirectory(name)) {
                editorBuffer = createNewEditorBuffer(name, name, fileModificationTime(name),
                                                     fileSize(name));
            }
        }
        if (editorBuffer != NULL && !isDirectory(editorBuffer->fileName)) {
            allocNewEditorBufferTextSpace(editorBuffer, fileSize(name));
            loadFileIntoEditorBuffer(editorBuffer, fileModificationTime(name), fileSize(name));
        } else {
            return NULL;
        }
    }
    return editorBuffer;
}

EditorBuffer *findEditorBufferForFileOrCreateEmpty(char *name) {
    EditorBuffer *buffer = findEditorBufferForFile(name);
    if (buffer == NULL) {
        buffer = createNewEditorBuffer(name, name, time(NULL), 0);
        assert(buffer!=NULL);
        allocNewEditorBufferTextSpace(buffer, 0);
        buffer->textLoaded = true;
    }
    return buffer;
}
