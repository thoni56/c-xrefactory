#include "editor.h"

#include <stdlib.h>
#include <ctype.h>
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


EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next) {
    EditorRegionList *regionList;

    regionList = malloc(sizeof(EditorRegionList));
    regionList->region.begin = begin;
    regionList->region.end = end;
    regionList->next = next;

    return regionList;
}


//////////////////////////////////////////////////////////////////////////////
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

static bool editorRegionListBefore(EditorRegionList *l1, EditorRegionList *l2) {
    if (editorMarkerBefore(l1->region.begin, l2->region.begin))
        return true;
    if (editorMarkerBefore(l2->region.begin, l1->region.begin))
        return false;
    // region beginnings are equal, check end
    if (editorMarkerBefore(l1->region.end, l2->region.end))
        return true;
    if (editorMarkerBefore(l2->region.end, l1->region.end))
        return false;
    return false;
}

void freeTextSpace(char *space, int index) {
    EditorMemoryBlock *sp;
    sp = (EditorMemoryBlock *) space;
    sp->next = editorMemory[index];
    editorMemory[index] = sp;
}

void loadFileIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime, size_t fileSize) {
    char *text = buffer->allocation.text;
    assert(text != NULL);

    int bufferSize = buffer->allocation.bufferSize;
    log_trace(":loading file %s==%s size %d", buffer->loadedFromFile, buffer->fileName, bufferSize);

    FILE *file = openFile(buffer->loadedFromFile, "r");
    if (file == NULL) {
        FATAL_ERROR(ERR_CANT_OPEN, buffer->loadedFromFile, XREF_EXIT_ERR);
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
            sprintf(tmpBuffer, "File %s: read %d chars of %d", buffer->loadedFromFile, bufferSize - size,
                    bufferSize);
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
                if (fileExists(l->buffer->loadedFromFile)) {
                    int size = fileSize(l->buffer->loadedFromFile);
                    allocateNewEditorBufferTextSpace(l->buffer, size);
                    loadFileIntoEditorBuffer(l->buffer,
                                             fileModificationTime(l->buffer->loadedFromFile), size);
                    log_trace("preloading %s into %s", l->buffer->loadedFromFile, l->buffer->fileName);
                }
            }
        }
    }
}

static bool runWithEditorMarkerUntil(EditorMarker *marker, int (*until)(int), int step) {
    int   offset, max;
    char *text;

    assert(step == -1 || step == 1);
    offset = marker->offset;
    max    = marker->buffer->allocation.bufferSize;
    text   = marker->buffer->allocation.text;
    while (offset >= 0 && offset < max && (*until)(text[offset]) == 0)
        offset += step;
    marker->offset = offset;
    if (offset < 0) {
        marker->offset = 0;
        return false;
    }
    if (offset >= max) {
        marker->offset = max - 1;
        return false;
    }
    return true;
}

int countLinesBetweenEditorMarkers(EditorMarker *m1, EditorMarker *m2) {
    // this can happen after an error in moving, just pass in this case
    if (m1 == NULL || m2 == NULL)
        return 0;
    assert(m1->buffer == m2->buffer);
    assert(m1->offset <= m2->offset);
    char *text  = m1->buffer->allocation.text;
    int   max   = m2->offset;
    int   count = 0;
    for (int i = m1->offset; i < max; i++) {
        if (text[i] == '\n')
            count++;
    }
    return count;
}

static int isNewLine(int c) {
    return c == '\n';
}
int        moveEditorMarkerToNewline(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNewLine, direction);
}

static int isNonBlank(int c) {return ! isspace(c);}
int editorMoveMarkerToNonBlank(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNonBlank, direction);
}

static int isNonBlankOrNewline(int c) {return c=='\n' || ! isspace(c);}
int        moveEditorMarkerToNonBlankOrNewline(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNonBlankOrNewline, direction);
}

static int isNotIdentPart(int c) {return !isalnum(c) && c!='_' && c!='$';}
int        moveEditorMarkerBeyondIdentifier(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNotIdentPart, direction);
}

void removeBlanksAtEditorMarker(EditorMarker *mm, int direction, EditorUndo **undo) {
    int moffset;

    moffset = mm->offset;
    if (direction < 0) {
        mm->offset --;
        editorMoveMarkerToNonBlank(mm, -1);
        mm->offset++;
        replaceStringInEditorBuffer(mm->buffer, mm->offset, moffset - mm->offset, "", 0, undo);
    } else if (direction > 0) {
        editorMoveMarkerToNonBlank(mm, 1);
        replaceStringInEditorBuffer(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);
    } else {
        // both directions
        mm->offset --;
        editorMoveMarkerToNonBlank(mm, -1);
        mm->offset++;
        moffset = mm->offset;
        editorMoveMarkerToNonBlank(mm, 1);
        replaceStringInEditorBuffer(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);
    }
}

EditorMarkerList *convertReferencesToEditorMarkers(Reference *references) {
    Reference        *reference;
    EditorMarkerList *markerList;

    markerList = NULL;
    reference   = references;
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

void freeEditorMarkersAndRegionList(EditorRegionList *occs) {
    for (EditorRegionList *o = occs; o != NULL;) {
        EditorRegionList *next = o->next; /* Save next as we are freeing 'o' */
        freeEditorMarker(o->region.begin);
        freeEditorMarker(o->region.end);
        free(o);
        o = next;
    }
}

#if 0
void editorDumpBuffer(EditorBuffer *buff) {
    /* TODO: Should really put this in log() */
    for (int i=0; i<buff->allocation.bufferSize; i++) {
        putc(buff->allocation.text[i], errOut);
    }
}

void editorDumpBuffers(void) {
    log_trace("[editorDumpBuffers] start");
    for (int i=0; i != -1 ; i = getNextExistingEditorBufferIndex(i+1)) {
        for (EditorBufferList *ll = getEditorBuffer(i); ll != NULL; ll = ll->next) {
            log_trace("%d : %s==%s, %d", i, ll->buffer->name, ll->buffer->fileName,
                      ll->buffer->textLoaded);
        }
    }
    log_trace("[editorDumpBuffers] end");
}

void editorDumpRegionList(EditorRegionList *mml) {
    log_trace("[dumping editor regions]");
    for (EditorRegionList *mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->region.begin == NULL || mm->region.end == NULL) {
            log_trace("%ld: [null]", (unsigned long)mm);
        } else {
            log_trace("%ld: [%s: %d - %d] --> %c - %c", (unsigned long)mm,
                      simpleFileName(mm->region.begin->buffer->name), mm->region.begin->offset,
                      mm->region.end->offset, CHAR_ON_MARKER(mm->region.begin), CHAR_ON_MARKER(mm->region.end));
        }
    }
    log_trace("[dumpend] of editor regions]");
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

static EditorMarkerList *combineEditorMarkerLists(EditorMarkerList **diff, EditorMarkerList *list) {
    EditorMarker     *marker = newEditorMarker(list->marker->buffer, list->marker->offset);
    EditorMarkerList *l;

    l = malloc(sizeof(EditorMarkerList));
    *l    = (EditorMarkerList){.marker = marker, .usage = list->usage, .next = *diff};
    *diff = l;
    list  = list->next;

    return list;
}

void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2,
                              EditorMarkerList **diff1, EditorMarkerList **diff2) {
    EditorMarkerList *l1, *l2;

    LIST_MERGE_SORT(EditorMarkerList, *list1, editorMarkerListBefore);
    LIST_MERGE_SORT(EditorMarkerList, *list2, editorMarkerListBefore);
    *diff1 = *diff2 = NULL;
    for(l1 = *list1, l2 = *list2; l1!=NULL && l2!=NULL; ) {
        if (editorMarkerListBefore(l1, l2)) {
            EditorMarker *marker = newEditorMarker(l1->marker->buffer, l1->marker->offset);
            EditorMarkerList *l;
            l = malloc(sizeof(EditorMarkerList)); /* TODO1 */
            *l = (EditorMarkerList){.marker = marker, .usage = l1->usage, .next = *diff1};
            *diff1 = l;
            l1 = l1->next;
        } else if (editorMarkerListBefore(l2, l1)) {
            l2 = combineEditorMarkerLists(diff2, l2);
        } else {
            l1 = l1->next;
            l2 = l2->next;
        }
    }
    while (l1 != NULL) {
        l1 = combineEditorMarkerLists(diff1, l1);
    }
    while (l2 != NULL) {
        EditorMarker *marker = newEditorMarker(l2->marker->buffer, l2->marker->offset);
        EditorMarkerList *l;
        l = malloc(sizeof(EditorMarkerList));
        *l = (EditorMarkerList){.marker = marker, .usage = l2->usage, .next = *diff2};
        *diff2 = l;
        l2 = l2->next;
    }
}

void sortEditorRegionsAndRemoveOverlaps(EditorRegionList **regions) {
    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListBefore);
    for (EditorRegionList *region = *regions; region != NULL; region = region->next) {
        EditorRegionList *next = region->next;
        if (next != NULL && region->region.begin->buffer == next->region.begin->buffer) {
            assert(region->region.begin->buffer
                   == region->region.end->buffer); // region consistency check
            assert(next->region.begin->buffer
                   == next->region.end->buffer); // region consistency check
            assert(region->region.begin->offset <= next->region.begin->offset);
            EditorMarker *newEnd = NULL;
            if (next->region.end->offset <= region->region.end->offset) {
                // second inside first
                newEnd = region->region.end;
                freeEditorMarker(next->region.begin);
                freeEditorMarker(next->region.end);
            } else if (next->region.begin->offset <= region->region.end->offset) {
                // they have common part
                newEnd = next->region.end;
                freeEditorMarker(next->region.begin);
                freeEditorMarker(region->region.end);
            }
            if (newEnd != NULL) {
                region->region.end = newEnd;
                region->next       = next->next;
                free(next);
                next = NULL;
                continue;
            }
        }
    }
}

void splitEditorMarkersWithRespectToRegions(EditorMarkerList **inMarkers,
                                            EditorRegionList **inRegions,
                                            EditorMarkerList **outInsiders,
                                            EditorMarkerList **outOutsiders) {
    EditorMarkerList *markers1, *markers2;
    EditorRegionList *regions;

    *outInsiders = NULL;
    *outOutsiders = NULL;

    LIST_MERGE_SORT(EditorMarkerList, *inMarkers, editorMarkerListBefore);
    sortEditorRegionsAndRemoveOverlaps(inRegions);

    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *inMarkers);

    //&editorDumpRegionList(*inRegions);
    //&editorDumpMarkerList(*inMarkers);

    regions = *inRegions;
    markers1= *inMarkers;
    while (markers1!=NULL) {
        markers2 = markers1->next;
        while (regions!=NULL && editorMarkerAfter(regions->region.begin, markers1->marker))
            regions = regions->next;
        if (regions!=NULL && editorMarkerAfter(regions->region.end, markers1->marker)) {
            // is inside
            markers1->next = *outInsiders;
            *outInsiders = markers1;
        } else {
            // is outside
            markers1->next = *outOutsiders;
            *outOutsiders = markers1;
        }
        markers1 = markers2;
    }

    *inMarkers = NULL;
    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *outInsiders);
    LIST_REVERSE(EditorMarkerList, *outOutsiders);
    //&editorDumpMarkerList(*outInsiders);
    //&editorDumpMarkerList(*outOutsiders);
}

void restrictEditorMarkersToRegions(EditorMarkerList **mm, EditorRegionList **regions) {
    EditorMarkerList *ins, *outs;
    splitEditorMarkersWithRespectToRegions(mm, regions, &ins, &outs);
    *mm = ins;
    freeEditorMarkerListAndMarkers(outs);
}

EditorMarker *createEditorMarkerForBufferBegin(EditorBuffer *buffer) {
    return newEditorMarker(buffer, 0);
}

EditorMarker *createEditorMarkerForBufferEnd(EditorBuffer *buffer) {
    return newEditorMarker(buffer, buffer->allocation.bufferSize);
}

EditorRegionList *createEditorRegionForWholeBuffer(EditorBuffer *buffer) {
    EditorMarker *bufferBegin, *bufferEnd;
    EditorRegion theBufferRegion;
    EditorRegionList *theBufferRegionList;

    bufferBegin     = createEditorMarkerForBufferBegin(buffer);
    bufferEnd       = createEditorMarkerForBufferEnd(buffer);
    theBufferRegion = (EditorRegion){.begin = bufferBegin, .end = bufferEnd};
    theBufferRegionList = malloc(sizeof(EditorRegionList));
    *theBufferRegionList = (EditorRegionList){.region = theBufferRegion, .next = NULL};

    return theBufferRegionList;
}

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
        && buffer->fileName==buffer->loadedFromFile  /* not -preloaded */
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
            log_trace("closable %d for %s(%d) %s(%d)", bufferIsCloseable(l->buffer), l->buffer->fileName,
                      l->buffer->fileName, l->buffer->loadedFromFile, l->buffer->loadedFromFile);
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
