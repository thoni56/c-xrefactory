#include "editorbuffer.h"

#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "editor.h"             /* For EditorMarker */
#include "editorbuffertable.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "memory.h"
#include "undo.h"


void fillEmptyEditorBuffer(EditorBuffer *buffer, char *fileName, int fileNumber, char *realFileName) {
    buffer->allocation = (EditorBufferAllocationData){
        .bufferSize = 0, .text = NULL, .allocatedFreePrefixSize = 0,
        .allocatedBlock = NULL, .allocatedIndex = 0,
        .allocatedSize = 0};
    *buffer = (EditorBuffer){
        .fileName = fileName, .fileNumber = fileNumber, .realFileName = realFileName,
        .markers = NULL, .allocation = buffer->allocation};
    buffer->modificationTime = 0;
    buffer->size = 0;
    buffer->textLoaded = false;
}

/**
 * @brief Registers an EditorBuffer in the editor buffer table
 *
 * This function allocates the necessary EditorBufferList element so
 * that the caller does not have to bother with that.
 *
 * @param buffer - buffer owned by the caller to register.
 * @return the index in the editor buffer table.
 */
int registerEditorBuffer(EditorBuffer *buffer) {
    EditorBufferList *list = malloc(sizeof(EditorBufferList));
    *list = (EditorBufferList){.buffer = buffer, .next = NULL};
    return addEditorBuffer(list);
}

EditorBuffer *newEditorBuffer(char *fileName, int fileNumber, char *realFileName, time_t modificationTime,
                              size_t size) {
    EditorBuffer *editorBuffer = malloc(sizeof(EditorBuffer));
    fillEmptyEditorBuffer(editorBuffer, fileName, fileNumber, realFileName);
    editorBuffer->modificationTime = modificationTime;
    editorBuffer->size = size;
    return editorBuffer;
}

static void checkForMagicMarker(EditorBufferAllocationData *allocation) {
    assert(allocation->allocatedBlock[allocation->allocatedSize] == 0x3b);
}

static void freeMarkersInEditorBuffer(EditorBuffer *buffer) {
    for (EditorMarker *marker = buffer->markers; marker != NULL;) {
        EditorMarker *next = marker->next;
        free(marker);
        marker = next;
    }
}

void freeEditorBuffer(EditorBuffer *buffer) {
    if (buffer == NULL)
        return;
    log_trace("freeing buffer %s==%s", buffer->fileName, buffer->realFileName);
    if (buffer->realFileName != buffer->fileName) {
        /* If the two are not pointing to the same string... */
        free(buffer->realFileName);
    }
    free(buffer->fileName);

    freeMarkersInEditorBuffer(buffer);

    if (buffer->textLoaded) {
        log_trace("freeing %d of size %d", buffer->allocation.allocatedBlock,
                  buffer->allocation.allocatedSize);
        checkForMagicMarker(&buffer->allocation);
        freeTextSpace(buffer->allocation.allocatedBlock,
                      buffer->allocation.allocatedIndex);
    }
    free(buffer);
}

EditorBuffer *createNewEditorBuffer(char *fileName, char *realFileName, time_t modificationTime,
                                    size_t size) {
    char *normalizedFileName, *normalizedRealFileName;
    EditorBuffer *buffer;
    EditorBufferList *bufferList;

    normalizedFileName = strdup(normalizeFileName_static(fileName, cwd));

    /* This is really a check if the file was preloaded from the
     * editor and not read from the original file */
    normalizedRealFileName = normalizeFileName_static(realFileName, cwd);
    if (strcmp(normalizedRealFileName, normalizedFileName)==0) {
        normalizedRealFileName = normalizedFileName;
    } else {
        normalizedRealFileName = strdup(normalizedRealFileName);
    }

    buffer = newEditorBuffer(normalizedFileName, 0, normalizedRealFileName, modificationTime, size);

    bufferList = malloc(sizeof(EditorBufferList));
    *bufferList = (EditorBufferList){.buffer = buffer, .next = NULL};
    log_trace("creating buffer '%s' for '%s'", buffer->fileName, buffer->realFileName);

    addEditorBuffer(bufferList);

    // set fileNumber last, because, addfiletabitem calls back the statb
    // from editor, so be tip-top at this moment!
    buffer->fileNumber = addFileNameToFileTable(normalizedFileName);

    return buffer;
}

EditorBuffer *getEditorBufferFor(char *name) {
    EditorBuffer editorBuffer;
    EditorBufferList editorBufferList, *foundElement;

    fillEmptyEditorBuffer(&editorBuffer, name, 0, name);
    editorBufferList = (EditorBufferList){.buffer = &editorBuffer, .next = NULL};
    if (editorBufferIsMember(&editorBufferList, NULL, &foundElement)) {
        return foundElement->buffer;
    }
    return NULL;
}

EditorBuffer *getOpenedAndLoadedEditorBuffer(char *name) {
    EditorBuffer *buffer;
    buffer = getEditorBufferFor(name);
    if (buffer!=NULL && buffer->textLoaded)
        return buffer;
    return NULL;
}

EditorBuffer *findEditorBufferForFile(char *name) {
    EditorBuffer *editorBuffer = getOpenedAndLoadedEditorBuffer(name);

    if (editorBuffer==NULL) {
        editorBuffer = getEditorBufferFor(name);
        if (editorBuffer == NULL) {
            if (fileExists(name) && !isDirectory(name)) {
                editorBuffer = createNewEditorBuffer(name, name, fileModificationTime(name),
                                                     fileSize(name));
            }
        }
        if (editorBuffer != NULL && !isDirectory(editorBuffer->realFileName)) {
            allocateNewEditorBufferTextSpace(editorBuffer, fileSize(name));
            loadFileIntoEditorBuffer(editorBuffer, fileModificationTime(name), fileSize(name));
        } else {
            return NULL;
        }
    }
    return editorBuffer;
}

// Only used from Options for preload
EditorBuffer *openEditorBufferFromPreload(char *name, char *fileName) {
    EditorBuffer  *buffer;

    buffer = getEditorBufferFor(name);
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
    EditorBuffer newBuffer;

    strcpy(newName, normalizeFileName_static(nName, cwd));
    fillEmptyEditorBuffer(&newBuffer, buffer->fileName, 0, buffer->fileName);

    EditorBufferList newBufferListElement = (EditorBufferList){.buffer = &newBuffer, .next = NULL};

    EditorBufferList *foundMember;
    if (!editorBufferIsMember(&newBufferListElement, NULL, &foundMember)) {
        char tmpBuffer[TMP_BUFF_SIZE];
        sprintf(tmpBuffer, "Trying to rename non existing buffer %s", buffer->fileName);
        errorMessage(ERR_INTERNAL, tmpBuffer);
        return;
    }
    assert(foundMember->buffer == buffer);
    bool deleted = deleteEditorBuffer(foundMember);
    assert(deleted);

    char *oldName = buffer->fileName;
    buffer->fileName = strdup(newName);

    // Also update fileNumber
    int newFileNumber = addFileNameToFileTable(newName);
    getFileItemWithFileNumber(newFileNumber)->isArgument = getFileItemWithFileNumber(buffer->fileNumber)->isArgument;
    buffer->fileNumber = newFileNumber;

    *foundMember = (EditorBufferList){.buffer = buffer, .next = NULL};
    EditorBufferList *memb2;
    if (editorBufferIsMember(foundMember, NULL, &memb2)) {
        deleteEditorBuffer(memb2);
        freeEditorBuffer(memb2->buffer);
        free(memb2);
    }
    addEditorBuffer(foundMember);

    // note undo operation
    if (undo!=NULL) {
        *undo = newUndoRename(buffer, oldName, *undo);
    }
    setEditorBufferModified(buffer);

    // finally create a buffer with old name and empty text in order
    // to keep information that the file is no longer existing
    // so old references will be removed on update (fixing problem of
    // of moving a package into an existing package).
    EditorBuffer *removed = createNewEditorBuffer(oldName, oldName, buffer->modificationTime, buffer->size);
    allocateNewEditorBufferTextSpace(removed, 0);
    removed->textLoaded = true;
    setEditorBufferModified(removed);
}
