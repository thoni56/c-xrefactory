#include "editor.h"

#include <stdlib.h>
#include <memory.h>

#include "commons.h"
#include "cxref.h"
#include "editorbuffer.h"
#include "editorbuffertable.h"
#include "encoding.h"
#include "fileio.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "proto.h"
#include "undo.h"
#include "usage.h"


typedef struct editorMemoryBlock {
    struct editorMemoryBlock *next;
} EditorMemoryBlock;


#define MIN_EDITOR_MEMORY_BLOCK_SIZE_BITS 11 /* 11 bits will be 2^11 bytes in size */
#define MAX_EDITOR_MEMORY_BLOCK_SIZE_BITS 32 /* 32 bits will be 2^32 bytes in size */

// this has to cover at least alignment allocations
#define EDITOR_ALLOCATION_RESERVE 1024
#define EDITOR_FREE_PREFIX_SIZE 16

/* 32 entries, only 11-32 is used, pointers to allocated areas.
   I have no idea why this is so complicated, why not just use
   allocated memory, why keep it in this array? */
static EditorMemoryBlock *editorMemory[MAX_EDITOR_MEMORY_BLOCK_SIZE_BITS];

#include "editorbuffertable.h"


void editorInit(void) {
    initEditorBufferTable();
}

time_t editorFileModificationTime(char *path) {
    EditorBuffer *buffer;

    buffer = getEditorBufferForFile(path);
    if (buffer != NULL)
        return buffer->modificationTime;
    return fileModificationTime(path);
}

size_t editorFileSize(char *path) {
    EditorBuffer *buffer;

    buffer = getEditorBufferForFile(path);
    if (buffer != NULL)
        return buffer->size;
    return fileSize(path);
}

bool editorFileExists(char *path) {
    EditorBuffer *buffer;

    buffer = getEditorBufferForFile(path);
    if (buffer != NULL)
        return true;
    return fileExists(path);
}

static void editorError(int errCode, char *message) {
    errorMessage(errCode, message);
}

void freeTextSpace(char *space, int index) {
    EditorMemoryBlock *sp;
    sp = (EditorMemoryBlock *) space;
    sp->next = editorMemory[index];
    editorMemory[index] = sp;
}

void loadTextIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime, const char *text) {
    allocateNewEditorBufferTextSpace(buffer, strlen(text));
    strcpy(buffer->allocation.text, text);
}

void loadFileIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime, size_t fileSize) {
    allocateNewEditorBufferTextSpace(buffer, fileSize);

    char *text = buffer->allocation.text;
    assert(text != NULL);

    assert(buffer->preLoadedFromFile == NULL || buffer->preLoadedFromFile != buffer->fileName);
    int bufferSize = buffer->allocation.bufferSize;
    char *fileName = buffer->preLoadedFromFile? buffer->preLoadedFromFile: buffer->fileName;

    log_trace(":loading file %s==%s size %d", fileName, buffer->fileName, bufferSize);

    FILE *file = openFile(fileName, "r");
    if (file == NULL) {
        FATAL_ERROR(ERR_CANT_OPEN, fileName, XREF_EXIT_ERR);
    }

    int size = bufferSize;
    int n;
    do {
        n    = readFile(file, text, 1, size);
        text = text + n;
        size = size - n;
    } while (n>0);
    closeFile(file);

    if (size != 0) {
        // this is possible, due to <CR><LF> conversion under MS-DOS
        buffer->allocation.bufferSize -= size;
        if (size < 0) {
            char tmpBuffer[TMP_BUFF_SIZE];
            sprintf(tmpBuffer, "File %s: read %d chars of %d", fileName, bufferSize - size, bufferSize);
            editorError(ERR_INTERNAL, tmpBuffer);
        }
    }
    performEncodingAdjustments(buffer);
    buffer->modificationTime = modificationTime;
    buffer->size = fileSize;
    buffer->textLoaded = true;
}

// Should really be in editorbuffer but is dependent on many editor things...
void allocateNewEditorBufferTextSpace(EditorBuffer *buffer, int size) {
    int minSize = size + EDITOR_ALLOCATION_RESERVE + EDITOR_FREE_PREFIX_SIZE;
    int allocIndex = 11;
    int allocatedSize = 2048;

    // Ensure size to allocate is at least
    for(; allocatedSize<minSize; ) {
        allocIndex++;
        allocatedSize = allocatedSize << 1;
    }

    char *space = (char *)editorMemory[allocIndex];
    if (space == NULL) {
        space = malloc(allocatedSize+1);
        if (space == NULL)
            FATAL_ERROR(ERR_NO_MEMORY, "global malloc", XREF_EXIT_ERR);
        // put magic
        space[allocatedSize] = 0x3b;
    } else {
        editorMemory[allocIndex] = editorMemory[allocIndex]->next;
    }
    buffer->allocation = (EditorBufferAllocationData){.bufferSize = size, .text = space+EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedFreePrefixSize = EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedBlock = space, .allocatedIndex = allocIndex,
                                           .allocatedSize = allocatedSize};
}

void replaceStringInEditorBuffer(EditorBuffer *buffer, int offset, int deleteSize, char *string,
                                 int length, EditorUndo **undoP) {
    assert(offset >=0 && offset <= buffer->allocation.bufferSize);
    assert(deleteSize >= 0);
    assert(length >= 0);

    int oldSize = buffer->allocation.bufferSize;
    if (deleteSize+offset > oldSize) {
        // deleting over end of buffer,
        // delete only until end of buffer
        deleteSize = oldSize - offset;
    }
    log_trace("replacing string in buffer %d (%s)", buffer, buffer->fileName);

    int newSize = oldSize + length - deleteSize;
    // prepare operation
    if (newSize >= buffer->allocation.allocatedSize - buffer->allocation.allocatedFreePrefixSize) {
        // resize buffer
        log_trace("resizing %s from %d(%d) to %d", buffer->fileName, buffer->allocation.bufferSize,
                  buffer->allocation.allocatedSize, newSize);
        char *text = buffer->allocation.text;
        char *space = buffer->allocation.allocatedBlock;
        int index = buffer->allocation.allocatedIndex;
        allocateNewEditorBufferTextSpace(buffer, newSize);
        memcpy(buffer->allocation.text, text, oldSize);
        buffer->allocation.bufferSize = oldSize;
        freeTextSpace(space, index);
    }

    assert(newSize < buffer->allocation.allocatedSize - buffer->allocation.allocatedFreePrefixSize);
    if (undoP!=NULL) {
        // note undo information
        int undoSize = length;
        assert(deleteSize >= 0);
        char *undoText = malloc(deleteSize+1);
        memcpy(undoText, buffer->allocation.text+offset, deleteSize);
        undoText[deleteSize]=0;
        EditorUndo *u = newUndoReplace(buffer, offset, undoSize, deleteSize, undoText, *undoP);
        *undoP = u;
    }

    // edit text
    memmove(buffer->allocation.text+offset+length, buffer->allocation.text+offset+deleteSize,
            buffer->allocation.bufferSize - offset - deleteSize);
    memcpy(buffer->allocation.text+offset, string, length);
    buffer->allocation.bufferSize = buffer->allocation.bufferSize - deleteSize + length;

    // update markers
    if (deleteSize > length) {
        int pattractor;
        if (length > 0)
            pattractor = offset + length - 1;
        else
            pattractor = offset + length;
        for (EditorMarker *m=buffer->markers; m!=NULL; m=m->next) {
            if (m->offset >= offset + length) {
                if (m->offset < offset+deleteSize) {
                    m->offset = pattractor;
                } else {
                    m->offset = m->offset - deleteSize + length;
                }
            }
        }
    } else {
        for (EditorMarker *m=buffer->markers; m!=NULL; m=m->next) {
            if (m->offset >= offset + deleteSize) {
                m->offset = m->offset - deleteSize + length;
            }
        }
    }
    setEditorBufferModified(buffer);
}

void moveBlockInEditorBuffer(EditorMarker *sourceMarker, EditorMarker *destinationMarker, int size,
                             EditorUndo **undo) {
    assert(size>=0);
    if (destinationMarker->buffer == sourceMarker->buffer
        && destinationMarker->offset > sourceMarker->offset
        && destinationMarker->offset < sourceMarker->offset+size) {
        errorMessage(ERR_INTERNAL, "[editor] moving block to its original place");
        return;
    }
    EditorBuffer *sourceBuffer = sourceMarker->buffer;
    EditorBuffer *destinationBuffer = destinationMarker->buffer;

    // insert the block to target position
    int destinationOffset = destinationMarker->offset;
    int offset1 = sourceMarker->offset;
    int offset2 = offset1+size;
    assert(offset1 <= offset2);

    // do it at two steps for the case if source buffer equals target buffer
    // first just allocate space
    replaceStringInEditorBuffer(destinationBuffer, destinationOffset, 0, sourceBuffer->allocation.text + offset1,
                                offset2 - offset1, NULL);

    // now copy text
    offset1 = sourceMarker->offset;
    offset2 = offset1+size;
    replaceStringInEditorBuffer(destinationBuffer, destinationOffset, offset2 - offset1,
                                sourceBuffer->allocation.text + offset1, offset2 - offset1, NULL);
    // save target for undo;
    int undoOffset = sourceMarker->offset;

    // move all markers from moved block
    assert(offset1 == sourceMarker->offset);
    assert(offset2 == offset1+size);
    EditorMarker *mm = sourceBuffer->markers;
    while (mm!=NULL) {
        EditorMarker *tmp = mm->next;
        if (mm->offset>=offset1 && mm->offset<offset2) {
            removeEditorMarkerFromBufferWithoutFreeing(mm);
            mm->offset = destinationOffset + (mm->offset-offset1);
            attachMarkerToBuffer(mm, destinationBuffer);
        }
        mm = tmp;
    }
    // remove the source block
    replaceStringInEditorBuffer(sourceBuffer, offset1, offset2 - offset1, sourceBuffer->allocation.text + offset1,
                                0, NULL);
    //
    setEditorBufferModified(sourceBuffer);
    setEditorBufferModified(destinationBuffer);

    // add the whole operation into undo
    if (undo!=NULL) {
        *undo = newUndoMove(destinationBuffer, sourceMarker->offset, offset2-offset1, sourceBuffer, undoOffset,
                            *undo);
    }
}

static void quasiSaveEditorBuffer(EditorBuffer *buffer) {
    buffer->modifiedSinceLastQuasiSave = false;
    buffer->modificationTime = time(NULL);
    FileItem *fileItem = getFileItemWithFileNumber(buffer->fileNumber);
    fileItem->lastModified = buffer->modificationTime;
}

void quasiSaveModifiedEditorBuffers(void) {
    bool          saving            = false;
    static time_t lastQuasiSaveTime = 0;

    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *ll = getEditorBufferListElementAt(i); ll != NULL; ll = ll->next) {
            if (ll->buffer->modifiedSinceLastQuasiSave) {
                saving = true;
                goto cont;
            }
        }
    }
cont:
    if (saving) {
        // sychronization, since last quazi save, there must
        // be at least one second, otherwise times will be wrong
        time_t currentTime = time(NULL);
        if (lastQuasiSaveTime > currentTime + 5) {
            FATAL_ERROR(ERR_INTERNAL, "last save in the future, travelling in time?",
                        XREF_EXIT_ERR);
        } else if (lastQuasiSaveTime >= currentTime) {
            sleep(1 + lastQuasiSaveTime - currentTime);
        }
    }
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *ll = getEditorBufferListElementAt(i); ll != NULL; ll = ll->next) {
            if (ll->buffer->modifiedSinceLastQuasiSave) {
                quasiSaveEditorBuffer(ll->buffer);
            }
        }
    }
    lastQuasiSaveTime = time(NULL);
}

void loadAllOpenedEditorBuffers(void) {
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            if (!l->buffer->textLoaded) {
                assert(l->buffer->preLoadedFromFile == NULL || l->buffer->preLoadedFromFile != l->buffer->fileName);
                char *fileName = l->buffer->preLoadedFromFile? l->buffer->preLoadedFromFile: l->buffer->fileName;
                if (fileExists(fileName)) {
                    loadFileIntoEditorBuffer(l->buffer, fileModificationTime(fileName), fileSize(fileName));
                    log_trace("loading '%s' into '%s'", fileName, l->buffer->fileName);
                }
            }
        }
    }
}

void removeBlanksAtEditorMarker(EditorMarker *marker, int direction, EditorUndo **undo) {
    int offset = marker->offset;
    if (direction < 0) {
        marker->offset--;
        moveEditorMarkerToNonBlank(marker, -1);
        marker->offset++;
        replaceStringInEditorBuffer(marker->buffer, marker->offset, offset - marker->offset, "", 0, undo);
    } else if (direction > 0) {
        moveEditorMarkerToNonBlank(marker, 1);
        replaceStringInEditorBuffer(marker->buffer, offset, marker->offset - offset, "", 0, undo);
    } else {
        // both directions
        marker->offset --;
        moveEditorMarkerToNonBlank(marker, -1);
        marker->offset++;
        offset = marker->offset;
        moveEditorMarkerToNonBlank(marker, 1);
        replaceStringInEditorBuffer(marker->buffer, offset, marker->offset - offset, "", 0, undo);
    }
}

EditorMarkerList *convertReferencesToEditorMarkers(Reference *references) {
    EditorMarkerList *markerList = NULL;
    Reference        *reference = references;

    while (reference != NULL) {
        while (reference != NULL && !isVisibleUsage(reference->usage))
            reference = reference->next;
        if (reference != NULL) {
            int           file     = reference->position.file;
            int           line     = reference->position.line;
            int           col      = reference->position.col;
            FileItem     *fileItem = getFileItemWithFileNumber(file);
            EditorBuffer *buff     = findOrCreateAndLoadEditorBufferForFile(fileItem->name);
            if (buff == NULL) {
                errorMessage(ERR_CANT_OPEN, fileItem->name);
                while (reference != NULL && file == reference->position.file)
                    reference = reference->next;
            } else {
                char *text      = buff->allocation.text;
                char *smax      = text + buff->allocation.bufferSize;
                int   maxoffset = buff->allocation.bufferSize - 1;
                if (maxoffset < 0)
                    maxoffset = 0;
                int l = 1;
                int c  = 0;
                for (; text < smax; text++, c++) {
                    if (l == line && c == col) {
                        EditorMarker *m    = newEditorMarker(buff, text - buff->allocation.text);
                        EditorMarkerList *rrr  = malloc(sizeof(EditorMarkerList));
                        *rrr = (EditorMarkerList){.marker = m, .usage = reference->usage, .next = markerList};
                        markerList  = rrr;
                        reference    = reference->next;
                        while (reference != NULL && !isVisibleUsage(reference->usage))
                            reference = reference->next;
                        if (reference == NULL || file != reference->position.file)
                            break;
                        line = reference->position.line;
                        col  = reference->position.col;
                    }
                    if (*text == '\n') {
                        l++;
                        c = -1;
                    }
                }
                // references beyond end of buffer
                while (reference != NULL && file == reference->position.file) {
                    EditorMarker *m    = newEditorMarker(buff, maxoffset);
                    EditorMarkerList *rrr  = malloc(sizeof(EditorMarkerList));
                    *rrr = (EditorMarkerList){.marker = m, .usage = reference->usage, .next = markerList};
                    markerList  = rrr;
                    reference    = reference->next;
                    while (reference != NULL && !isVisibleUsage(reference->usage))
                        reference = reference->next;
                }
            }
        }
    }
    // get markers in the same order as were references
    // ?? is this still needed?
    LIST_REVERSE(EditorMarkerList, markerList);
    return markerList;
}

Reference *convertEditorMarkersToReferences(EditorMarkerList **editorMarkerListP) {
    EditorMarkerList *markers;
    EditorBuffer     *buf;
    char             *text, *textMax, *offset;
    int               line, col;
    Reference        *reference;

    LIST_MERGE_SORT(EditorMarkerList, *editorMarkerListP, editorMarkerListBefore);
    reference = NULL;
    markers = *editorMarkerListP;
    while (markers!=NULL) {
        buf = markers->marker->buffer;
        text = buf->allocation.text;
        textMax = text + buf->allocation.bufferSize;
        offset = buf->allocation.text + markers->marker->offset;
        line = 1; col = 0;
        for (; text<textMax; text++, col++) {
            if (text == offset) {
                reference = newReference((Position){buf->fileNumber, line, col}, markers->usage, reference);
                markers = markers->next;
                if (markers==NULL || markers->marker->buffer != buf)
                    break;
                offset = buf->allocation.text + markers->marker->offset;
            }
            if (*text=='\n') {
                line++;
                col = -1;
            }
        }
        while (markers!=NULL && markers->marker->buffer==buf) {
            reference = newReference((Position){buf->fileNumber, line, 0}, markers->usage, reference);
            markers = markers->next;
        }
    }
    LIST_MERGE_SORT(Reference, reference, olcxReferenceInternalLessFunction);
    return reference;
}

#if 0
void editorDumpBuffer(EditorBuffer *buff) {
    /* TODO: Should really put this in log() */
    for (int i=0; i<buff->allocation.bufferSize; i++) {
        putc(buff->allocation.text[i], errOut);
    }
}

void editorDumpUndoList(EditorUndo *undo) {
    log_trace("[dumping editor undo list]");
    while (undo != NULL) {
        switch (undo->operation) {
        case UNDO_REPLACE_STRING:
            log_trace("replace string [%s:%d] %d (%ld)%s %d", undo->buffer->name, undo->u.replace.offset,
                      undo->u.replace.size, (unsigned long)undo->u.replace.str, undo->u.replace.str,
                      undo->u.replace.strlen);
            if (strlen(undo->u.replace.str) != undo->u.replace.strlen)
                log_trace("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            break;
        case UNDO_RENAME_BUFFER:
            log_trace("rename buffer %s %s", undo->buffer->name, undo->u.rename.name);
            break;
        case UNDO_MOVE_BLOCK:
            log_trace("move block [%s:%d] [%s:%d] size==%d", undo->buffer->name, undo->u.moveBlock.offset,
                      undo->u.moveBlock.dbuffer->name, undo->u.moveBlock.doffset, undo->u.moveBlock.size);
            break;
        default:
            errorMessage(ERR_INTERNAL, "Unknown operation to undo");
        }
        undo = undo->next;
    }
    log_trace("[undodump] end");
}
#endif

static EditorBufferList *computeListOfAllEditorBuffers(void) {
    EditorBufferList *list = NULL;
    for (int i=0; i != -1 ; i = getNextExistingEditorBufferIndex(i+1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            EditorBufferList *element;
            element = malloc(sizeof(EditorBufferList));
            *element = (EditorBufferList){.buffer = l->buffer, .next = list};
            list     = element;
        }
    }
    return list;
}

static void freeEditorBufferListButNotBuffers(EditorBufferList *list) {
    EditorBufferList *l, *n;

    l=list;
    while (l!=NULL) {
        n = l->next;
        free(l);
        l = n;
    }
}

static int editorBufferNameLess(EditorBufferList*l1, EditorBufferList*l2) {
    return strcmp(l1->buffer->fileName, l2->buffer->fileName);
}

static bool directoryPrefixMatches(char *fileName, char *dirname) {
    int dirNameLength = strlen(dirname);
    return filenameCompare(fileName, dirname, dirNameLength) == 0
        && (fileName[dirNameLength] == '/'
            || fileName[dirNameLength] == '\\');
}

// TODO, do all this stuff better!
// This is still quadratic on number of opened buffers
// for recursive search
bool editorMapOnNonExistantFiles(char *dirname,
                                void (*fun)(MAP_FUN_SIGNATURE),
                                SearchDepth depth,
                                char *a1,
                                char *a2,
                                Completions *a3,
                                void *a4,
                                int *a5
) {
    // In order to avoid mapping of the same directory several
    // times, first just create list of all files, sort it, and then
    // map them
    EditorBufferList *listOfAllBuffers   = computeListOfAllEditorBuffers();
    LIST_MERGE_SORT(EditorBufferList, listOfAllBuffers, editorBufferNameLess);

    EditorBufferList *list = listOfAllBuffers;

    bool found = false;
    while(list!=NULL) {
        if (directoryPrefixMatches(list->buffer->fileName, dirname)) {
            int dirNameLength = strlen(dirname);
            char fname[MAX_FILE_NAME_SIZE];
            int fileNameLength;
            if (depth == DEPTH_ONE) {
                char *pathDelimiter = strchr(list->buffer->fileName+dirNameLength+1, '/');
                if (pathDelimiter==NULL)
                    pathDelimiter = strchr(list->buffer->fileName+dirNameLength+1, '\\');
                if (pathDelimiter==NULL) {
                    strcpy(fname, list->buffer->fileName+dirNameLength+1);
                    fileNameLength = strlen(fname);
                } else {
                    fileNameLength = pathDelimiter-(list->buffer->fileName+dirNameLength+1);
                    strncpy(fname, list->buffer->fileName+dirNameLength+1, fileNameLength);
                    fname[fileNameLength]=0;
                }
            } else {
                strcpy(fname, list->buffer->fileName+dirNameLength+1);
                fileNameLength = strlen(fname);
            }
            // Only map on nonexistant files
            if (!fileExists(list->buffer->fileName)) {
                // get file name
                (*fun)(fname, a1, a2, a3, a4, a5);
                found = true;
                // skip all files in the same directory
                char *lastMapped = list->buffer->fileName;
                int lastMappedLength = dirNameLength+1+fileNameLength;
                list = list->next;
                while (list!=NULL
                       && filenameCompare(list->buffer->fileName, lastMapped, lastMappedLength)==0
                       && (list->buffer->fileName[lastMappedLength]=='/' || list->buffer->fileName[lastMappedLength]=='\\')) {
                    list = list->next;
                }
            } else {
                list = list->next;
            }
        } else {
            list = list->next;
        }
    }
    freeEditorBufferListButNotBuffers(listOfAllBuffers);

    return found;
}

static bool bufferIsCloseable(EditorBuffer *buffer) {
    return buffer != NULL && buffer->textLoaded && buffer->markers==NULL
        && buffer->preLoadedFromFile == NULL  /* not -preloaded */
        && ! buffer->modified;
}

void closeEditorBufferIfCloseable(char *name) {
    EditorBuffer *buffer = findOrCreateAndLoadEditorBufferForFile(name);
    if (bufferIsCloseable(buffer))
        deregisterEditorBuffer(name);
    freeEditorBuffer(buffer);
}

void closeAllEditorBuffersIfClosable(void) {
    EditorBufferList *allEditorBuffers = computeListOfAllEditorBuffers();
    for (EditorBufferList *l = allEditorBuffers; l!=NULL; l=l->next) {
        if (bufferIsCloseable(l->buffer)) {
            log_trace("closable %d for '%s'='%s'", bufferIsCloseable(l->buffer),
                      l->buffer->preLoadedFromFile?l->buffer->preLoadedFromFile:"(null)", l->buffer->fileName);
            EditorBuffer *buffer = deregisterEditorBuffer(l->buffer->fileName);
            freeEditorBuffer(buffer);
        }
    }
    freeEditorBufferListButNotBuffers(allEditorBuffers);
}

void closeAllEditorBuffers(void) {
    EditorBufferList *allEditorBuffers = computeListOfAllEditorBuffers();
    for (EditorBufferList *l = allEditorBuffers; l!=NULL; l=l->next) {
        EditorBuffer *buffer = deregisterEditorBuffer(l->buffer->fileName);
        freeEditorBuffer(buffer);
    }
    freeEditorBufferListButNotBuffers(allEditorBuffers);
}
