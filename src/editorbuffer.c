#include "editorbuffer.h"

#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "editorbuffertab.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "memory.h"
#include "undo.h"


void fillEmptyEditorBuffer(EditorBuffer *buffer, char *name, int fileNumber, char *fileName) {
    buffer->allocation = (EditorBufferAllocationData){.bufferSize = 0, .text = NULL, .allocatedFreePrefixSize = 0,
                                                      .allocatedBlock = NULL, .allocatedIndex = 0,
                                                      .allocatedSize = 0};
    *buffer = (EditorBuffer){.name = name, .fileNumber = fileNumber, .fileName = fileName, .markers = NULL,
                             .allocation = buffer->allocation};
    buffer->modificationTime = 0;
    buffer->size = 0;
}

static void checkForMagicMarker(EditorBufferAllocationData *allocation) {
    assert(allocation->allocatedBlock[allocation->allocatedSize] == 0x3b);
}


void freeEditorBuffer(EditorBufferList *list) {
    if (list == NULL)
        return;
    log_trace("freeing buffer %s==%s", list->buffer->name, list->buffer->fileName);
    if (list->buffer->fileName != list->buffer->name) {
        free(list->buffer->fileName);
    }
    free(list->buffer->name);
    for (EditorMarker *marker=list->buffer->markers; marker!=NULL;) {
        EditorMarker *next = marker->next;
        free(marker);
        marker = next;
    }
    if (list->buffer->textLoaded) {
        log_trace("freeing %d of size %d", list->buffer->allocation.allocatedBlock,
                  list->buffer->allocation.allocatedSize);
        checkForMagicMarker(&list->buffer->allocation);
        freeTextSpace(list->buffer->allocation.allocatedBlock,
                      list->buffer->allocation.allocatedIndex);
    }
    free(list->buffer);
    free(list);
}

EditorBuffer *createNewEditorBuffer(char *name, char *fileName, time_t modificationTime,
                                    size_t size) {
    char *allocatedName, *normalizedName, *afname, *normalizedFileName;
    EditorBuffer *buffer;
    EditorBufferList *bufferList;

    normalizedName = normalizeFileName_static(name, cwd);
    allocatedName = malloc(strlen(normalizedName)+1);
    strcpy(allocatedName, normalizedName);
    normalizedFileName = normalizeFileName_static(fileName, cwd);
    if (strcmp(normalizedFileName, allocatedName)==0) {
        afname = allocatedName;
    } else {
        afname = malloc(strlen(normalizedFileName)+1);
        strcpy(afname, normalizedFileName);
    }
    buffer = malloc(sizeof(EditorBuffer));
    fillEmptyEditorBuffer(buffer, allocatedName, 0, afname);
    buffer->modificationTime = modificationTime;
    buffer->size = size;

    bufferList = malloc(sizeof(EditorBufferList));
    *bufferList = (EditorBufferList){.buffer = buffer, .next = NULL};
    log_trace("creating buffer '%s' for '%s'", buffer->name, buffer->fileName);

    addEditorBuffer(bufferList);

    // set ftnum at the end, because, addfiletabitem calls back the statb
    // from editor, so be tip-top at this moment!
    buffer->fileNumber = addFileNameToFileTable(allocatedName);

    return buffer;
}

EditorBuffer *getOpenedEditorBuffer(char *name) {
    EditorBuffer editorBuffer;
    EditorBufferList editorBufferList, *element;

    fillEmptyEditorBuffer(&editorBuffer, name, 0, name);
    editorBufferList = (EditorBufferList){.buffer = &editorBuffer, .next = NULL};
    if (editorBufferIsMember(&editorBufferList, NULL, &element)) {
        return element->buffer;
    }
    return NULL;
}

EditorBuffer *getOpenedAndLoadedEditorBuffer(char *name) {
    EditorBuffer *res;
    res = getOpenedEditorBuffer(name);
    if (res!=NULL && res->textLoaded)
        return res;
    return NULL;
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

// Only used from Options for preload
EditorBuffer *openEditorBufferNoFileLoad(char *name, char *fileName) {
    EditorBuffer  *buffer;

    buffer = getOpenedEditorBuffer(name);
    if (buffer != NULL) {
        return buffer;
    }
    buffer = createNewEditorBuffer(name, fileName, fileModificationTime(fileName),
                                   fileSize(fileName));
    return buffer;
}

void setEditorBufferModified(EditorBuffer *buffer) {
    buffer->modified = true;
    buffer->modifiedSinceLastQuasiSave = true;
}

void renameEditorBuffer(EditorBuffer *buffer, char *nName, EditorUndo **undo) {
    char newName[MAX_FILE_NAME_SIZE];
    int fileNumber, deleted;
    EditorBuffer dd, *removed;
    EditorBufferList ddl, *memb, *memb2;
    char *oldName;

    strcpy(newName, normalizeFileName_static(nName, cwd));
    log_trace("Renaming %s (at %d) to %s (at %d)", buffer->name, buffer->name, newName, newName);
    fillEmptyEditorBuffer(&dd, buffer->name, 0, buffer->name);
    ddl = (EditorBufferList){.buffer = &dd, .next = NULL};
    if (!editorBufferIsMember(&ddl, NULL, &memb)) {
        char tmpBuffer[TMP_BUFF_SIZE];
        sprintf(tmpBuffer, "Trying to rename non existing buffer %s", buffer->name);
        errorMessage(ERR_INTERNAL, tmpBuffer);
        return;
    }
    assert(memb->buffer == buffer);
    deleted = deleteEditorBuffer(memb);
    assert(deleted);
    oldName = buffer->name;
    buffer->name = malloc(strlen(newName)+1);
    strcpy(buffer->name, newName);

    // Also update fileNumber
    fileNumber = addFileNameToFileTable(newName);
    getFileItem(fileNumber)->isArgument = getFileItem(buffer->fileNumber)->isArgument;
    buffer->fileNumber = fileNumber;

    *memb = (EditorBufferList){.buffer = buffer, .next = NULL};
    if (editorBufferIsMember(memb, NULL, &memb2)) {
        deleteEditorBuffer(memb2);
        freeEditorBuffer(memb2);
    }
    addEditorBuffer(memb);

    // note undo operation
    if (undo!=NULL) {
        *undo = newUndoRename(buffer, oldName, *undo);
    }
    setEditorBufferModified(buffer);

    // finally create a buffer with old name and empty text in order
    // to keep information that the file is no longer existing
    // so old references will be removed on update (fixing problem of
    // of moving a package into an existing package).
    removed = createNewEditorBuffer(oldName, oldName, buffer->modificationTime, buffer->size);
    allocNewEditorBufferTextSpace(removed, 0);
    removed->textLoaded = true;
    setEditorBufferModified(removed);
}
