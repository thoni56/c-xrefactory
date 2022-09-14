#include "editor.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "yylex.h"
#include "misc.h"
#include "cxref.h"
#include "list.h"
#include "log.h"
#include "fileio.h"
#include "filetable.h"
#include "editorbuffertab.h"

#include "protocol.h"

typedef struct editorMemoryBlock {
    struct editorMemoryBlock *next;
} EditorMemoryBlock;

EditorUndo *editorUndo = NULL;

#define MIN_EDITOR_MEMORY_BLOCK 11
#define MAX_EDITOR_MEMORY_BLOCK 32

// this has to cover at least alignment allocations
#define EDITOR_ALLOCATION_RESERVE 1024
#define EDITOR_FREE_PREFIX_SIZE 16

static EditorMemoryBlock *editorMemory[MAX_EDITOR_MEMORY_BLOCK];

#include "editorbuffertab.h"

////////////////////////////////////////////////////////////////////
//                      encoding stuff

#define EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, command) {    \
        unsigned char *s, *d, *maxs;                            \
        unsigned char *space;                                   \
        space = (unsigned char *)buff->allocation.text;         \
        maxs = space + buff->allocation.bufferSize;             \
        for(s=d=space; s<maxs; s++) {                           \
            command                                             \
                }                                               \
        buff->allocation.bufferSize = d - space;                \
    }

#define EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)    \
    if (*s == '\r') {                               \
        if (s+1<maxs && *(s+1)=='\n') {             \
            s++;                                    \
            *d++ = *s;                              \
        } else {                                    \
            *d++ = '\n';                            \
        }                                           \
    }
#define EDITOR_ENCODING_CR_LF_CONVERSION(s,d)       \
    if (*s == '\r' && s+1<maxs && *(s+1)=='\n') {   \
        s++;                                        \
        *d++ = *s;                                  \
    }
#define EDITOR_ENCODING_CR_CONVERSION(s,d)      \
    if (*s == '\r') {                           \
        *d++ = '\n';                            \
    }
#define EDITOR_ENCODING_UTF8_CONVERSION(s,d)    \
    if (*(s) & 0x80) {                          \
        unsigned z;                             \
        z = *s;                                 \
        if (z <= 223) {s+=1; *d++ = ' ';}       \
        else if (z <= 239) {s+=2; *d++ = ' ';}  \
        else if (z <= 247) {s+=3; *d++ = ' ';}  \
        else if (z <= 251) {s+=4; *d++ = ' ';}  \
        else {s+=5; *d++ = ' ';}                \
    }
#define EDITOR_ENCODING_EUC_CONVERSION(s,d)     \
    if (*(s) & 0x80) {                          \
        unsigned z;                             \
        z = *s;                                 \
        if (z == 0x8e) {s+=2; *d++ = ' ';}      \
        else if (z == 0x8f) {s+=3; *d++ = ' ';} \
        else {s+=1; *d++ = ' ';}                \
    }
#define EDITOR_ENCODING_SJIS_CONVERSION(s,d)        \
    if (*(s) & 0x80) {                              \
        unsigned z;                                 \
        z = *s;                                     \
        if (z >= 0xa1 && z <= 0xdf) {*d++ = ' ';}   \
        else {s+=1; *d++ = ' ';}                    \
    }
#define EDITOR_ENCODING_ELSE_BRANCH(s,d)        \
    {                                           \
        *d++ = *s;                              \
    }

static void applyCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applyCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applyCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_CR_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applyUtf8CrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyUtf8CrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyUtf8CrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyUtf8Conversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applyEucCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyEucCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyEucCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applyEucConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applySjisCrLfCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applySjisCrLfConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applySjisCrConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void applySjisConversion(EditorBuffer *buffer) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buffer, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void applyUtf16Conversion(EditorBuffer *buffer) {
    unsigned char  *s, *d, *maxs;
    unsigned int cb, cb2;
    int little_endian;
    unsigned char *space;

    space = (unsigned char *)buffer->allocation.text;
    maxs = space + buffer->allocation.bufferSize;
    s = space;
    // determine endian first
    cb = (*s << 8) + *(s+1);
    if (cb == 0xfeff) {
        little_endian = 0;
        s += 2;
    } else if (cb == 0xfffe) {
        little_endian = 1;
        s += 2;
    } else if (options.fileEncoding == MULE_UTF_16LE) {
        little_endian = 1;
    } else if (options.fileEncoding == MULE_UTF_16BE) {
        little_endian = 0;
    } else {
        little_endian = 1;
    }
    for(s++,d=space; s<maxs; s+=2) {
        if (little_endian) cb = (*(s) << 8) + *(s-1);
        else cb = (*(s-1) << 8) + *(s);
        if (cb != 0xfeff) {
            if (cb >= 0xd800 && cb <= 0xdfff) {
                // 32 bit character
                s += 2;
                if (little_endian) cb2 = (*(s) << 8) + *(s-1);
                else cb2 = (*(s-1) << 8) + *(s);
                cb = 0x10000 + ((cb & 0x3ff) << 10) + (cb2 & 0x3ff);
            }
            if (cb < 0x80) {
                *d++ = (char)(cb & 0xff);
            } else {
                *d++ = ' ';
            }
        }
    }
    buffer->allocation.bufferSize = d - space;
}

static bool bufferStartsWithUtf16Bom(EditorBuffer *buffer) {
    unsigned char *s;

    s = (unsigned char *)buffer->allocation.text;
    if (buffer->allocation.bufferSize >= 2) {
        unsigned cb = (*s << 8) + *(s+1);
        if (cb == 0xfeff || cb == 0xfffe)
            return true;
    }
    return false;
}

static void performSimpleLineFeedConversion(EditorBuffer *buffer) {
    if ((options.eolConversion&CR_LF_EOL_CONVERSION)
        && (options.eolConversion & CR_EOL_CONVERSION)) {
        applyCrLfCrConversion(buffer);
    } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
        applyCrLfConversion(buffer);
    } else if (options.eolConversion & CR_EOL_CONVERSION) {
        applyCrConversion(buffer);
    }
}

static void performEncodingAdjustments(EditorBuffer *buffer) {
    // do different loops for efficiency reasons
    if (options.fileEncoding == MULE_EUROPEAN) {
        performSimpleLineFeedConversion(buffer);
    } else if (options.fileEncoding == MULE_EUC) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            applyEucCrLfCrConversion(buffer);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            applyEucCrLfConversion(buffer);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            applyEucCrConversion(buffer);
        } else {
            applyEucConversion(buffer);
        }
    } else if (options.fileEncoding == MULE_SJIS) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            applySjisCrLfCrConversion(buffer);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            applySjisCrLfConversion(buffer);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            applySjisCrConversion(buffer);
        } else {
            applySjisConversion(buffer);
        }
    } else {
        // default == utf
        if ((options.fileEncoding != MULE_UTF_8 && bufferStartsWithUtf16Bom(buffer))
            || options.fileEncoding == MULE_UTF_16 || options.fileEncoding == MULE_UTF_16LE
            || options.fileEncoding == MULE_UTF_16BE) {
            applyUtf16Conversion(buffer);
            performSimpleLineFeedConversion(buffer);
        } else {
            // utf-8
            if ((options.eolConversion&CR_LF_EOL_CONVERSION)
                && (options.eolConversion & CR_EOL_CONVERSION)) {
                applyUtf8CrLfCrConversion(buffer);
            } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
                applyUtf8CrLfConversion(buffer);
            } else if (options.eolConversion & CR_EOL_CONVERSION) {
                applyUtf8CrConversion(buffer);
            } else {
                applyUtf8Conversion(buffer);
            }
        }
    }
}

static void attachMarkerToBuffer(EditorMarker *marker, EditorBuffer *buffer) {
    marker->buffer = buffer;
    marker->next = buffer->markers;
    buffer->markers = marker;
    marker->previous = NULL;
    if (marker->next!=NULL)
        marker->next->previous = marker;
}

EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset) {
    EditorMarker *marker;

    marker = editorAlloc(sizeof(EditorMarker));
    marker->buffer = buffer;
    marker->offset = offset;
    marker->previous = NULL;
    marker->next = NULL;

    attachMarkerToBuffer(marker, buffer);

    return marker;
}

EditorMarker *newEditorMarkerForPosition(Position *position) {
    EditorBuffer *buffer;
    EditorMarker *marker;

    if (position->file==noFileIndex || position->file<0) {
        errorMessage(ERR_INTERNAL, "[editor] creating marker for non-existent position");
    }
    buffer = findEditorBufferForFile(getFileItem(position->file)->name);
    marker = newEditorMarker(buffer, 0);
    moveEditorMarkerToLineAndColumn(marker, position->line, position->col);
    return marker;
}

EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next) {
    EditorRegionList *regionList;

    regionList = editorAlloc(sizeof(EditorRegionList));
    regionList->region.begin = begin;
    regionList->region.end = end;
    regionList->next = next;

    return regionList;
}


//////////////////////////////////////////////////////////////////////////////
void editorInit(void) {
    initEditorBufferTable();
}

int editorFileStatus(char *path, struct stat *statP) {
    EditorBuffer *buffer;

    buffer = getOpenedEditorBuffer(path);
    if (buffer != NULL) {
        if (statP != NULL) {
            //*statP = buffer->stat; // We don't have this anymore
            log_trace("returning stat of %s modified at %s", path, ctime(&buffer->modificationTime));
        }
        return 0;
    }
    return fileStatus(path, statP);
}

time_t editorFileModificationTime(char *path) {
    EditorBuffer *buffer;

    buffer = getOpenedEditorBuffer(path);
    if (buffer != NULL)
        return buffer->modificationTime;
    return fileModificationTime(path);
}

size_t editorFileSize(char *path) {
    EditorBuffer *buffer;

    buffer = getOpenedEditorBuffer(path);
    if (buffer != NULL)
        return buffer->size;
    return fileSize(path);
}

bool editorFileExists(char *path) {
    EditorBuffer *buffer;

    buffer = getOpenedEditorBuffer(path);
    if (buffer != NULL)
        return true;
    return fileExists(path);
}

static void editorError(int errCode, char *message) {
    errorMessage(errCode, message);
}

bool editorMarkerLess(EditorMarker *m1, EditorMarker *m2) {
    // m1->buffer->ftnum <> m2->buffer->ftnum;
    // following is tricky as it works also for renamed buffers
    if (m1->buffer < m2->buffer) return true;
    if (m1->buffer > m2->buffer) return false;
    if (m1->offset < m2->offset) return true;
    if (m1->offset > m2->offset) return false;
    return false;
}

bool editorMarkerLessOrEq(EditorMarker *m1, EditorMarker *m2) {
    return !editorMarkerLess(m2, m1);
}

bool editorMarkerGreater(EditorMarker *m1, EditorMarker *m2) {
    return editorMarkerLess(m2, m1);
}

bool editorMarkerListLess(EditorMarkerList *l1, EditorMarkerList *l2) {
    return editorMarkerLess(l1->marker, l2->marker);
}

bool editorRegionListLess(EditorRegionList *l1, EditorRegionList *l2) {
    if (editorMarkerLess(l1->region.begin, l2->region.begin)) return true;
    if (editorMarkerLess(l2->region.begin, l1->region.begin)) return false;
    // region beginnings are equal, check end
    if (editorMarkerLess(l1->region.end, l2->region.end)) return true;
    if (editorMarkerLess(l2->region.end, l1->region.end)) return false;
    return false;
}

EditorMarker *duplicateEditorMarker(EditorMarker *marker) {
    return newEditorMarker(marker->buffer, marker->offset);
}

static void removeEditorMarkerFromBufferWithoutFreeing(EditorMarker *marker) {
    if (marker == NULL) return;
    if (marker->next!=NULL) marker->next->previous = marker->previous;
    if (marker->previous==NULL) {
        marker->buffer->markers = marker->next;
    } else {
        marker->previous->next = marker->next;
    }
}

void freeEditorMarker(EditorMarker *marker) {
    if (marker == NULL)
        return;
    removeEditorMarkerFromBufferWithoutFreeing(marker);
    editorFree(marker, sizeof(EditorMarker));
}

static void freeTextSpace(char *space, int index) {
    EditorMemoryBlock *sp;
    sp = (EditorMemoryBlock *) space;
    sp->next = editorMemory[index];
    editorMemory[index] = sp;
}

static void checkForMagicMarker(EditorBufferAllocationData *allocation) {
    assert(allocation->allocatedBlock[allocation->allocatedSize] == 0x3b);
}

static void freeEditorBuffer(EditorBufferList *list) {
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

static void loadFileIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime,
                                     size_t fileSize) {
    char *text, *fname;
    FILE *file;
    int   n, size, bufferSize;

    fname = buffer->fileName;
    text       = buffer->allocation.text;
    bufferSize = buffer->allocation.bufferSize;
    log_trace(":loading file %s==%s size %d", fname, buffer->name, bufferSize);

#if defined (__WIN32__)
    file = openFile(fname, "r");         // was rb, but did not work
#else
    file = openFile(fname, "r");
#endif
    if (file == NULL) {
        FATAL_ERROR(ERR_CANT_OPEN, fname, XREF_EXIT_ERR);
    }
    size = bufferSize;
    assert(text != NULL);
    do {
        n    = readFile(text, 1, size, file);
        text = text + n;
        size = size - n;
    } while (n>0);
    closeFile(file);

    if (size != 0) {
        // this is possible, due to <CR><LF> conversion under MS-DOS
        buffer->allocation.bufferSize -= size;
        if (size < 0) {
            char tmpBuffer[TMP_BUFF_SIZE];
            sprintf(tmpBuffer, "File %s: read %d chars of %d", fname, bufferSize - size,
                    bufferSize);
            editorError(ERR_INTERNAL, tmpBuffer);
        }
    }
    performEncodingAdjustments(buffer);
    buffer->modificationTime = modificationTime;
    buffer->size = fileSize;
    buffer->textLoaded = true;
}

static void allocNewEditorBufferTextSpace(EditorBuffer *buffer, int size) {
    int minSize, allocIndex, allocSize;
    char *space;
    minSize = size + EDITOR_ALLOCATION_RESERVE + EDITOR_FREE_PREFIX_SIZE;
    allocIndex = 11; allocSize = 2048;
    for(; allocSize<minSize; ) {
        allocIndex++;
        allocSize = allocSize << 1;
    }
    space = (char *)editorMemory[allocIndex];
    if (space == NULL) {
        space = malloc(allocSize+1);
        if (space == NULL)
            FATAL_ERROR(ERR_NO_MEMORY, "global malloc", XREF_EXIT_ERR);
        // put magic
        space[allocSize] = 0x3b;
    } else {
        editorMemory[allocIndex] = editorMemory[allocIndex]->next;
    }
    buffer->allocation = (EditorBufferAllocationData){.bufferSize = size, .text = space+EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedFreePrefixSize = EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedBlock = space, .allocatedIndex = allocIndex,
                                           .allocatedSize = allocSize};
}

static void fillEmptyEditorBuffer(EditorBuffer *buffer, char *name, int ftnum, char *fileName) {
    buffer->allocation = (EditorBufferAllocationData){.bufferSize = 0, .text = NULL, .allocatedFreePrefixSize = 0,
                                                      .allocatedBlock = NULL, .allocatedIndex = 0,
                                                      .allocatedSize = 0};
    *buffer = (EditorBuffer){.name = name, .fileIndex = ftnum, .fileName = fileName, .markers = NULL,
                             .allocation = buffer->allocation};
    buffer->modificationTime = 0;
    buffer->size = 0;
}

static EditorBuffer *createNewEditorBuffer(char *name, char *fileName, time_t modificationTime,
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
    buffer->fileIndex = addFileNameToFileTable(allocatedName);

    return buffer;
}

static void setEditorBufferModified(EditorBuffer *buffer) {
    buffer->modified = true;
    buffer->modifiedSinceLastQuasiSave = true;
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

static EditorUndo *newEditorUndoReplace(EditorBuffer *buffer, unsigned offset, unsigned size,
                                          unsigned length, char *str, struct editorUndo *next) {
    EditorUndo *undo;

    undo = editorAlloc(sizeof(EditorUndo));
    undo->buffer = buffer;
    undo->operation = UNDO_REPLACE_STRING;
    undo->u.replace.offset = offset;
    undo->u.replace.size = size;
    undo->u.replace.strlen = length;
    undo->u.replace.str = str;
    undo->next = next;

    return undo;
}

static EditorUndo *newEditorUndoRename(EditorBuffer *buffer, char *name,
                                         struct editorUndo *next) {
    EditorUndo *undo;

    undo = editorAlloc(sizeof(EditorUndo));
    undo->buffer = buffer;
    undo->operation = UNDO_RENAME_BUFFER;
    undo->u.rename.name = name;
    undo->next = next;

    return undo;
}

static EditorUndo *newEditorUndoMove(EditorBuffer *buffer, unsigned offset, unsigned size,
                                     EditorBuffer *dbuffer, unsigned doffset,
                                     struct editorUndo *next) {
    EditorUndo *undo;

    undo = editorAlloc(sizeof(EditorUndo));
    undo->buffer = buffer;
    undo->operation = UNDO_MOVE_BLOCK;
    undo->u.moveBlock.offset = offset;
    undo->u.moveBlock.size = size;
    undo->u.moveBlock.dbuffer = dbuffer;
    undo->u.moveBlock.doffset = doffset;
    undo->next = next;

    return undo;
}

void renameEditorBuffer(EditorBuffer *buffer, char *nName, EditorUndo **undo) {
    char newName[MAX_FILE_NAME_SIZE];
    int fileIndex, deleted;
    EditorBuffer dd, *removed;
    EditorBufferList ddl, *memb, *memb2;
    char *oldName;

    strcpy(newName, normalizeFileName(nName, cwd));
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
    buffer->name = editorAlloc(strlen(newName)+1);
    strcpy(buffer->name, newName);
    // update also ftnum
    fileIndex = addFileNameToFileTable(newName);
    getFileItem(fileIndex)->isArgument = getFileItem(buffer->fileIndex)->isArgument;
    buffer->fileIndex = fileIndex;

    *memb = (EditorBufferList){.buffer = buffer, .next = NULL};
    if (editorBufferIsMember(memb, NULL, &memb2)) {
        deleteEditorBuffer(memb2);
        freeEditorBuffer(memb2);
    }
    addEditorBuffer(memb);

    // note undo operation
    if (undo!=NULL) {
        *undo = newEditorUndoRename(buffer, oldName, *undo);
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

EditorBuffer *findEditorBufferForFileOrCreate(char *name) {
    EditorBuffer *buffer = findEditorBufferForFile(name);
    if (buffer == NULL) {
        buffer = createNewEditorBuffer(name, name, time(NULL), 0);
        assert(buffer!=NULL);
        allocNewEditorBufferTextSpace(buffer, 0);
        buffer->textLoaded = true;
    }
    return buffer;
}

void replaceStringInEditorBuffer(EditorBuffer *buffer, int position, int delsize, char *str,
                                 int strlength, EditorUndo **undo) {
    int nsize, oldsize, index, undosize, pattractor;
    char *text, *space, *undotext;
    EditorMarker *m;
    EditorUndo *uu;

    assert(position >=0 && position <= buffer->allocation.bufferSize);
    assert(delsize >= 0);
    assert(strlength >= 0);
    oldsize = buffer->allocation.bufferSize;
    if (delsize+position > oldsize) {
        // deleting over end of buffer,
        // delete only until end of buffer
        delsize = oldsize - position;
    }
    log_trace("replacing string in buffer %d (%s)", buffer, buffer->name);
    nsize = oldsize + strlength - delsize;
    // prepare operation
    if (nsize >= buffer->allocation.allocatedSize - buffer->allocation.allocatedFreePrefixSize) {
        // resize buffer
        log_trace("resizing %s from %d(%d) to %d", buffer->name, buffer->allocation.bufferSize,
                  buffer->allocation.allocatedSize, nsize);
        text = buffer->allocation.text;
        space = buffer->allocation.allocatedBlock; index = buffer->allocation.allocatedIndex;
        allocNewEditorBufferTextSpace(buffer, nsize);
        memcpy(buffer->allocation.text, text, oldsize);
        buffer->allocation.bufferSize = oldsize;
        freeTextSpace(space, index);
    }
    assert(nsize < buffer->allocation.allocatedSize - buffer->allocation.allocatedFreePrefixSize);
    if (undo!=NULL) {
        // note undo information
        undosize = strlength;
        assert(delsize >= 0);
        undotext = editorAlloc(delsize+1);
        memcpy(undotext, buffer->allocation.text+position, delsize);
        undotext[delsize]=0;
        uu = newEditorUndoReplace(buffer, position, undosize, delsize, undotext, *undo);
        *undo = uu;
    }
    // edit text
    memmove(buffer->allocation.text+position+strlength, buffer->allocation.text+position+delsize,
            buffer->allocation.bufferSize - position - delsize);
    memcpy(buffer->allocation.text+position, str, strlength);
    buffer->allocation.bufferSize = buffer->allocation.bufferSize - delsize + strlength;
    //&sprintf(tmpBuffer,"setting buffersize of  %s to %d\n", buffer->name, buffer->allocation.bufferSize);ppcGenRecord(PPC_INFORMATION, tmpBuffer);fflush(communicationChannel);
    // update markers
    if (delsize > strlength) {
        if (strlength > 0) pattractor = position + strlength - 1;
        else pattractor = position + strlength;
        for(m=buffer->markers; m!=NULL; m=m->next) {
            if (m->offset >= position + strlength) {
                if (m->offset < position+delsize) {
                    m->offset = pattractor;
                } else {
                    m->offset = m->offset - delsize + strlength;
                }
            }
        }
    } else {
        for(m=buffer->markers; m!=NULL; m=m->next) {
            if (m->offset >= position + delsize) {
                m->offset = m->offset - delsize + strlength;
            }
        }
    }
    setEditorBufferModified(buffer);
}

void moveBlockInEditorBuffer(EditorMarker *dest, EditorMarker *src, int size,
                             EditorUndo **undo) {
    EditorMarker *tmp, *mm;
    EditorBuffer *sb, *db;
    int off1, off2, offd, undodoffset;

    assert(size>=0);
    if (dest->buffer == src->buffer
        && dest->offset > src->offset
        && dest->offset < src->offset+size) {
        errorMessage(ERR_INTERNAL, "[editor] moving block to its original place");
        return;
    }
    sb = src->buffer;
    db = dest->buffer;
    // insert the block to target position
    offd = dest->offset;
    off1 = src->offset;
    off2 = off1+size;
    assert(off1 <= off2);
    // do it at two steps for the case if source buffer equals target buffer
    // first just allocate space
    replaceStringInEditorBuffer(db, offd, 0, sb->allocation.text + off1, off2 - off1, NULL);
    // now copy text
    off1 = src->offset;
    off2 = off1+size;
    replaceStringInEditorBuffer(db, offd, off2 - off1, sb->allocation.text + off1, off2 - off1,
                                NULL);
    // save target for undo;
    undodoffset = src->offset;
    // move all markers from moved block
    assert(off1 == src->offset);
    assert(off2 == off1+size);
    mm = sb->markers;
    while (mm!=NULL) {
        tmp = mm->next;
        if (mm->offset>=off1 && mm->offset<off2) {
            removeEditorMarkerFromBufferWithoutFreeing(mm);
            mm->offset = offd + (mm->offset-off1);
            attachMarkerToBuffer(mm, db);
        }
        mm = tmp;
    }
    // remove the source block
    replaceStringInEditorBuffer(sb, off1, off2 - off1, sb->allocation.text + off1, 0, NULL);
    //
    setEditorBufferModified(sb);
    setEditorBufferModified(db);
    // add the whole operation into undo
    if (undo!=NULL) {
        *undo = newEditorUndoMove(db, src->offset, off2-off1, sb, undodoffset, *undo);
    }
}

static void quasiSaveEditorBuffer(EditorBuffer *buffer) {
    buffer->modifiedSinceLastQuasiSave = false;
    buffer->modificationTime = time(NULL);
    FileItem *fileItem = getFileItem(buffer->fileIndex);
    fileItem->lastModified = buffer->modificationTime;
}

void quasiSaveModifiedEditorBuffers(void) {
    bool          saving            = false;
    static time_t lastQuasiSaveTime = 0;

    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *ll = getEditorBuffer(i); ll != NULL; ll = ll->next) {
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
        for (EditorBufferList *ll = getEditorBuffer(i); ll != NULL; ll = ll->next) {
            if (ll->buffer->modifiedSinceLastQuasiSave) {
                quasiSaveEditorBuffer(ll->buffer);
            }
        }
    }
    lastQuasiSaveTime = time(NULL);
}

void loadAllOpenedEditorBuffers(void) {
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBuffer(i); l != NULL; l = l->next) {
            if (!l->buffer->textLoaded) {
                if (fileExists(l->buffer->fileName)) {
                    int size = fileSize(l->buffer->fileName);
                    allocNewEditorBufferTextSpace(l->buffer, size);
                    loadFileIntoEditorBuffer(l->buffer,
                                             fileModificationTime(l->buffer->fileName), size);
                    log_trace("preloading %s into %s", l->buffer->fileName, l->buffer->name);
                }
            }
        }
    }
}

bool runWithEditorMarkerUntil(EditorMarker *marker, int (*until)(int), int step) {
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

void moveEditorMarkerToLineAndColumn(EditorMarker *marker, int line, int col) {
    char           *t, *smax;
    int    ln;
    EditorBuffer   *buffer;
    int             c;

    assert(marker);
    buffer = marker->buffer;
    t      = buffer->allocation.text;
    smax   = t + buffer->allocation.bufferSize;
    ln = 1;
    if (line > 1) {
        for (; t < smax; t++) {
            if (*t == '\n') {
                ln++;
                if (ln == line)
                    break;
            }
        }
        if (t < smax)
            t++;
    }
    c = 0;
    for (; t < smax && c < col; t++, c++) {
        if (*t == '\n')
            break;
    }
    marker->offset = t - buffer->allocation.text;
    assert(marker->offset >= 0 && marker->offset <= buffer->allocation.bufferSize);
}

EditorMarkerList *convertReferencesToEditorMarkers(Reference *refs,
                                                   bool (*filter)(Reference *, void *),
                                                   void *filterParam) {
    Reference         *r;
    EditorMarker      *m;
    EditorMarkerList  *res, *rrr;
    int                 line, col, file, maxoffset;
    char       *s, *smax;
    int        ln, c;
    EditorBuffer      *buff;

    res = NULL;
    r = refs;
    while (r!=NULL) {
        while (r!=NULL && !filter(r,filterParam))
            r = r->next;
        if (r != NULL) {
            file = r->position.file;
            line = r->position.line;
            col = r->position.col;
            FileItem *fileItem = getFileItem(file);
            buff               = findEditorBufferForFile(fileItem->name);
            if (buff==NULL) {
                errorMessage(ERR_CANT_OPEN, fileItem->name);
                while (r!=NULL && file == r->position.file) r = r->next;
            } else {
                s = buff->allocation.text;
                smax = s + buff->allocation.bufferSize;
                maxoffset = buff->allocation.bufferSize - 1;
                if (maxoffset < 0) maxoffset = 0;
                ln = 1; c = 0;
                for(; s<smax; s++, c++) {
                    if (ln==line && c==col) {
                        m = newEditorMarker(buff, s - buff->allocation.text);
                        rrr = editorAlloc(sizeof(EditorMarkerList));
                        *rrr = (EditorMarkerList){.marker = m, .usage = r->usage, .next = res};
                        res = rrr;
                        r = r->next;
                        while (r!=NULL && ! filter(r,filterParam)) r = r->next;
                        if (r==NULL || file != r->position.file) break;
                        line = r->position.line;
                        col = r->position.col;
                    }
                    if (*s=='\n') {ln++; c = -1;}
                }
                // references beyond end of buffer
                while (r!=NULL && file == r->position.file) {
                    m = newEditorMarker(buff, maxoffset);
                    rrr = editorAlloc(sizeof(EditorMarkerList));
                    *rrr = (EditorMarkerList){.marker = m, .usage = r->usage, .next = res};
                    res = rrr;
                    r = r->next;
                    while (r != NULL && !filter(r, filterParam))
                        r = r->next;
                }
            }
        }
    }
    // get markers in the same order as were references
    // ?? is this still needed?
    LIST_REVERSE(EditorMarkerList, res);
    return res;
}

Reference *convertEditorMarkersToReferences(EditorMarkerList **editorMarkerListP) {
    EditorMarkerList *markers;
    EditorBuffer     *buf;
    char             *s, *smax, *offset;
    int               line, col;
    Reference        *reference;

    LIST_MERGE_SORT(EditorMarkerList, *editorMarkerListP, editorMarkerListLess);
    reference = NULL;
    markers = *editorMarkerListP;
    while (markers!=NULL) {
        buf = markers->marker->buffer;
        s = buf->allocation.text;
        smax = s + buf->allocation.bufferSize;
        offset = buf->allocation.text + markers->marker->offset;
        line = 1; col = 0;
        for (; s<smax; s++, col++) {
            if (s == offset) {
                Reference *r = olcx_alloc(sizeof(Reference));
                r->position = makePosition(buf->fileIndex, line, col);
                fillReference(r, markers->usage, r->position, reference);
                reference = r;
                markers = markers->next;
                if (markers==NULL || markers->marker->buffer != buf)
                    break;
                offset = buf->allocation.text + markers->marker->offset;
            }
            if (*s=='\n') {
                line++;
                col = -1;
            }
        }
        while (markers!=NULL && markers->marker->buffer==buf) {
            Reference *r = olcx_alloc(sizeof(Reference));
            r->position = makePosition(buf->fileIndex, line, 0);
            fillReference(r, markers->usage, r->position, reference);
            reference = r;
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
        editorFree(o, sizeof(EditorRegionList));
        o = next;
    }
}

void freeEditorMarkerListButNotMarkers(EditorMarkerList *occs) {
    for (EditorMarkerList *o = occs; o != NULL;) {
        EditorMarkerList *next = o->next; /* Save next as we are freeing 'o' */
        editorFree(o, sizeof(EditorMarkerList));
        o = next;
    }
}

void freeEditorMarkersAndMarkerList(EditorMarkerList *occs) {
    for (EditorMarkerList *o = occs; o != NULL;) {
        EditorMarkerList *next = o->next; /* Save next as we are freeing 'o' */
        freeEditorMarker(o->marker);
        editorFree(o, sizeof(EditorMarkerList));
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

void editorDumpMarker(EditorMarker *mm) {
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->buffer->name), mm->offset, CHAR_ON_MARKER(mm));
    ppcBottomInformation(tmpBuff);
}

void editorDumpMarkerList(EditorMarkerList *mml) {
    log_trace("[dumping editor markers]");
    for (EditorMarkerList *mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->marker == NULL) {
            log_trace("[null]");
        } else {
            log_trace("[%s:%d] --> %c", simpleFileName(mm->marker->buffer->name), mm->marker->offset,
                      CHAR_ON_MARKER(mm->marker));
        }
    }
    log_trace("[dumpend of editor marker]");
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

    l = editorAlloc(sizeof(EditorMarkerList));
    *l    = (EditorMarkerList){.marker = marker, .usage = list->usage, .next = *diff};
    *diff = l;
    list  = list->next;

    return list;
}

void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2,
                              EditorMarkerList **diff1, EditorMarkerList **diff2) {
    EditorMarkerList *l1, *l2;

    LIST_MERGE_SORT(EditorMarkerList, *list1, editorMarkerListLess);
    LIST_MERGE_SORT(EditorMarkerList, *list2, editorMarkerListLess);
    *diff1 = *diff2 = NULL;
    for(l1 = *list1, l2 = *list2; l1!=NULL && l2!=NULL; ) {
        if (editorMarkerListLess(l1, l2)) {
            EditorMarker *marker = newEditorMarker(l1->marker->buffer, l1->marker->offset);
            EditorMarkerList *l;
            l = editorAlloc(sizeof(EditorMarkerList)); /* TODO1 */
            *l = (EditorMarkerList){.marker = marker, .usage = l1->usage, .next = *diff1};
            *diff1 = l;
            l1 = l1->next;
        } else if (editorMarkerListLess(l2, l1)) {
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
        l = editorAlloc(sizeof(EditorMarkerList));
        *l = (EditorMarkerList){.marker = marker, .usage = l2->usage, .next = *diff2};
        *diff2 = l;
        l2 = l2->next;
    }
}

void sortEditorRegionsAndRemoveOverlaps(EditorRegionList **regions) {
    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListLess);
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
                editorFree(next, sizeof(EditorRegionList));
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

    LIST_MERGE_SORT(EditorMarkerList, *inMarkers, editorMarkerListLess);
    sortEditorRegionsAndRemoveOverlaps(inRegions);

    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *inMarkers);

    //&editorDumpRegionList(*inRegions);
    //&editorDumpMarkerList(*inMarkers);

    regions = *inRegions;
    markers1= *inMarkers;
    while (markers1!=NULL) {
        markers2 = markers1->next;
        while (regions!=NULL && editorMarkerGreater(regions->region.begin, markers1->marker))
            regions = regions->next;
        if (regions!=NULL && editorMarkerGreater(regions->region.end, markers1->marker)) {
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
    freeEditorMarkersAndMarkerList(outs);
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
    theBufferRegionList = editorAlloc(sizeof(EditorRegionList));
    *theBufferRegionList = (EditorRegionList){.region = theBufferRegion, .next = NULL};

    return theBufferRegionList;
}

static EditorBufferList *computeListOfAllEditorBuffers(void) {
    EditorBufferList *list;

    list = NULL;
    for (int i=0; i != -1 ; i = getNextExistingEditorBufferIndex(i+1)) {
        for (EditorBufferList *l = getEditorBuffer(i); l != NULL; l = l->next) {
            EditorBufferList *element;
            element = editorAlloc(sizeof(EditorBufferList));
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
        editorFree(l, sizeof(EditorBufferList));
        l = n;
    }
}

static int editorBufferNameLess(EditorBufferList*l1,EditorBufferList*l2) {
    return strcmp(l1->buffer->name, l2->buffer->name);
}

// TODO, do all this stuff better!
// This is still quadratic on number of opened buffers
// for recursive search
int editorMapOnNonexistantFiles(char *dirname,
                                void (*fun)(MAP_FUN_SIGNATURE),
                                int depth,
                                char *a1,
                                char *a2,
                                Completions *a3,
                                void *a4,
                                int *a5
) {
    int dlen, fnlen, res, lastMappedLen;
    EditorBufferList *ll, *bl;
    char *ss, *lastMapped;
    char fname[MAX_FILE_NAME_SIZE];

    // In order to avoid mapping of the same directory several
    // times, first just create list of all files, sort it, and then
    // map them
    res = 0;
    dlen = strlen(dirname);
    bl   = computeListOfAllEditorBuffers();
    LIST_MERGE_SORT(EditorBufferList, bl, editorBufferNameLess);
    ll = bl;
    //&sprintf(tmpBuffer, "ENTER!!!"); ppcGenRecord(PPC_IGNORE,tmpBuffer);
    while(ll!=NULL) {
        if (filenameCompare(ll->buffer->name, dirname, dlen)==0
            && (ll->buffer->name[dlen]=='/' || ll->buffer->name[dlen]=='\\')) {
            if (depth == DEPTH_ONE) {
                ss = strchr(ll->buffer->name+dlen+1, '/');
                if (ss==NULL) ss = strchr(ll->buffer->name+dlen+1, '\\');
                if (ss==NULL) {
                    strcpy(fname, ll->buffer->name+dlen+1);
                    fnlen = strlen(fname);
                } else {
                    fnlen = ss-(ll->buffer->name+dlen+1);
                    strncpy(fname, ll->buffer->name+dlen+1, fnlen);
                    fname[fnlen]=0;
                }
            } else {
                strcpy(fname, ll->buffer->name+dlen+1);
                fnlen = strlen(fname);
            }
            // Only map on nonexistant files
            if (!fileExists(ll->buffer->name)) {
                // get file name
                //&sprintf(tmpBuffer, "MAPPING %s as %s in %s", ll->buffer->name, fname, dirname); ppcGenRecord(PPC_IGNORE,tmpBuffer);
                (*fun)(fname, a1, a2, a3, a4, a5);
                res = 1;
                // skip all files in the same directory
                lastMapped = ll->buffer->name;
                lastMappedLen = dlen+1+fnlen;
                ll = ll->next;
                while (ll!=NULL
                       && filenameCompare(ll->buffer->name, lastMapped, lastMappedLen)==0
                       && (ll->buffer->name[lastMappedLen]=='/' || ll->buffer->name[lastMappedLen]=='\\')) {
                    //&sprintf(tmpBuffer, "SKIPPING %s", ll->buffer->name); ppcGenRecord(PPC_IGNORE,tmpBuffer);
                    ll = ll->next;
                }
            } else {
                ll = ll->next;
            }
        } else {
            ll = ll->next;
        }
    }
    freeEditorBufferListButNotBuffers(bl);
    //&sprintf(tmpBuffer, "QUIT!!!"); ppcGenRecord(PPC_IGNORE,tmpBuffer);
    return res;
}

static void closeEditorBuffer(EditorBufferList *member, int index) {
    log_trace("closing buffer %s:%s", member->buffer->name, member->buffer->fileName);

    EditorBufferList *l;
    for (l = getEditorBuffer(index); l != NULL; l = l->next)
        if (l == member)
            break;
    if (l == member) {
        // O.K. now, free the buffer
        setEditorBuffer(index, l->next);
        freeEditorBuffer(member);
    }
}

#define BUFFER_IS_CLOSABLE(buffer) (buffer->textLoaded             \
                                    && buffer->markers==NULL            \
                                    && buffer->name==buffer->fileName  /* not -preloaded */ \
                                    && ! buffer->modified          \
    )

// be very carefull when using this function, because of interpretation
// of 'Closable', this should be additional field: 'closable' or what
void closeEditorBufferIfClosable(char *name) {
    EditorBuffer dd;
    EditorBufferList ddl, *memb;
    int index;

    fillEmptyEditorBuffer(&dd, name, 0, name);
    ddl = (EditorBufferList){.buffer = &dd, .next = NULL};
    if (editorBufferIsMember(&ddl, &index, &memb)) {
        if (BUFFER_IS_CLOSABLE(memb->buffer)) {
            closeEditorBuffer(memb, index);
        }
    }
}

void closeAllEditorBuffersIfClosable(void) {
    for (int i=0; i != -1; i = getNextExistingEditorBufferIndex(i+1)) {
        for (EditorBufferList *list=getEditorBuffer(i); list!=NULL;) {
            EditorBufferList *next = list->next;
            log_trace("closable %d for %s(%d) %s(%d)", BUFFER_IS_CLOSABLE(list->buffer), list->buffer->name,
                      list->buffer->name, list->buffer->fileName, list->buffer->fileName);
            if (BUFFER_IS_CLOSABLE(list->buffer))
                closeEditorBuffer(list, i);
            list = next;
        }
    }
}

void closeAllEditorBuffers(void) {
    for (int i=0; i != -1; i = getNextExistingEditorBufferIndex(i+1)) {
        for (EditorBufferList *ll=getEditorBuffer(i); ll!=NULL;) {
            EditorBufferList *next = ll->next;
            freeEditorBuffer(ll);
            ll = next;
        }
        clearEditorBuffer(i);
    }
}
