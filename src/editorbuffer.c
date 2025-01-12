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


static void fillEmptyEditorBuffer(EditorBuffer *buffer, char *realFileName, int fileNumber, char *loadedFromFile) {
    buffer->allocation = (EditorBufferAllocationData){
        .bufferSize = 0, .text = NULL, .allocatedFreePrefixSize = 0,
        .allocatedBlock = NULL, .allocatedIndex = 0,
        .allocatedSize = 0};
    *buffer = (EditorBuffer){
        .fileName = realFileName, .fileNumber = fileNumber, .loadedFromFile = loadedFromFile,
        .markers = NULL, .allocation = buffer->allocation};
    buffer->modificationTime = 0;
    buffer->size = 0;
    buffer->textLoaded = false;
}

EditorBuffer *newEditorBuffer(char *realFileName, int fileNumber, char *loadedFromFile, time_t modificationTime,
                              size_t size) {
    EditorBuffer *editorBuffer = malloc(sizeof(EditorBuffer));
    fillEmptyEditorBuffer(editorBuffer, realFileName, fileNumber, loadedFromFile);
    editorBuffer->modificationTime = modificationTime;
    editorBuffer->size = size;
    return editorBuffer;
}

EditorBuffer *createNewEditorBuffer(char *realFileName, char *loadedFromFile, time_t modificationTime,
                                    size_t size) {
    char *normalizedRealFileName, *normalizedLoadedFromFile;
    EditorBuffer *buffer;

    normalizedRealFileName = strdup(normalizeFileName_static(realFileName, cwd));

    /* This is really a check if the file was preloaded from the
     * editor and not read from the original file */
    normalizedLoadedFromFile = normalizeFileName_static(loadedFromFile, cwd);
    if (strcmp(normalizedLoadedFromFile, normalizedRealFileName)==0) {
        normalizedLoadedFromFile = normalizedRealFileName;
    } else {
        normalizedLoadedFromFile = strdup(normalizedLoadedFromFile);
    }

    buffer = newEditorBuffer(normalizedRealFileName, 0, normalizedLoadedFromFile, modificationTime, size);
    log_trace("created buffer '%s' for '%s'", buffer->fileName, buffer->loadedFromFile);

    registerEditorBuffer(buffer);

    // set fileNumber last, because, addfiletabitem calls back the statb
    // from editor, so be tip-top at this moment!
    buffer->fileNumber = addFileNameToFileTable(normalizedRealFileName);

    return buffer;
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
    log_trace("freeing buffer %s==%s", buffer->fileName, buffer->loadedFromFile);
    if (buffer->loadedFromFile != buffer->fileName) {
        /* If the two are not pointing to the same string... */
        free(buffer->loadedFromFile);
    }
    free(buffer->fileName);

    freeMarkersInEditorBuffer(buffer);

    if (buffer->textLoaded) {
        log_trace("freeing allocated block %d of size %d", buffer->allocation.allocatedBlock,
                  buffer->allocation.allocatedSize);
        checkForMagicMarker(&buffer->allocation);
        freeTextSpace(buffer->allocation.allocatedBlock,
                      buffer->allocation.allocatedIndex);
    }
    free(buffer);
}

EditorBuffer *getOpenedAndLoadedEditorBuffer(char *fileName) {
    EditorBuffer *buffer = getEditorBufferForFile(fileName);
    if (buffer!=NULL && buffer->textLoaded)
        return buffer;
    return NULL;
}

EditorBuffer *findOrCreateAndLoadEditorBufferForFile(char *fileName) {
    EditorBuffer *editorBuffer = getEditorBufferForFile(fileName);

    if (editorBuffer != NULL && editorBuffer->textLoaded)
        /* Found one with text loaded, return it */
        return editorBuffer;

    if (editorBuffer == NULL) {
        /* No buffer found, create one unless it is a directory */
        if (fileExists(fileName) && !isDirectory(fileName)) {
            editorBuffer = createNewEditorBuffer(fileName, fileName, fileModificationTime(fileName),
                                                 fileSize(fileName));
            allocateNewEditorBufferTextSpace(editorBuffer, fileSize(fileName));
            loadFileIntoEditorBuffer(editorBuffer, fileModificationTime(fileName), fileSize(fileName));
        } else
            return NULL;
    }

    return editorBuffer;
}

// Only used from Options for preload
EditorBuffer *openEditorBufferFromPreload(char *fileName, char *loadedFromFile) {
    EditorBuffer  *buffer;

    buffer = getEditorBufferForFile(fileName);
    if (buffer != NULL) {
        return buffer;
    }
    buffer = createNewEditorBuffer(fileName, loadedFromFile, fileModificationTime(loadedFromFile),
                                   fileSize(loadedFromFile));
    return buffer;
}

void setEditorBufferModified(EditorBuffer *buffer) {
    buffer->modified = true;
    buffer->modifiedSinceLastQuasiSave = true;
}

void renameEditorBuffer(EditorBuffer *buffer, char *newName, EditorUndo **undo) {
    char newFileName[MAX_FILE_NAME_SIZE];

    strcpy(newFileName, normalizeFileName_static(newName, cwd));

    EditorBuffer *existing_buffer = getEditorBufferForFile(buffer->fileName);
    if (existing_buffer == NULL) {
        char tmpBuffer[TMP_BUFF_SIZE];
        sprintf(tmpBuffer, "Trying to rename non existing buffer %s", buffer->fileName);
        errorMessage(ERR_INTERNAL, tmpBuffer);
        return;
    }
    assert(existing_buffer == buffer);

    EditorBuffer *deregistered_original = deregisterEditorBuffer(buffer->fileName);
    assert(deregistered_original == buffer);

    char *oldName = buffer->fileName;
    buffer->fileName = strdup(newFileName);

    // Update fileNumber
    int newFileNumber = addFileNameToFileTable(newFileName);
    getFileItemWithFileNumber(newFileNumber)->isArgument = getFileItemWithFileNumber(buffer->fileNumber)->isArgument;
    buffer->fileNumber = newFileNumber;

    EditorBuffer *existing_buffer_with_the_new_name = getEditorBufferForFile(newFileName);
    if (existing_buffer_with_the_new_name != NULL) {
        EditorBuffer *deleted = deregisterEditorBuffer(newFileName);
        freeEditorBuffer(deleted);
    }
    registerEditorBuffer(buffer);

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
