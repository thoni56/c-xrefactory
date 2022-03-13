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

#include "protocol.h"

typedef struct editorMemoryBlock {
    struct editorMemoryBlock *next;
} S_editorMemoryBlock;

EditorUndo *s_editorUndo = NULL;

#define MIN_EDITOR_MEMORY_BLOCK 11
#define MAX_EDITOR_MEMORY_BLOCK 32

// this has to cover at least alignment allocations
#define EDITOR_ALLOCATION_RESERVE 1024
#define EDITOR_FREE_PREFIX_SIZE 16

S_editorMemoryBlock *s_editorMemory[MAX_EDITOR_MEMORY_BLOCK];

#include "editorbuffertab.h"

////////////////////////////////////////////////////////////////////
//                      encoding stuff

#define EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, command) {  \
        unsigned char *s, *d, *maxs;                          \
        unsigned char *space;                                 \
        space = (unsigned char *)buff->allocation.text;                \
        maxs = space + buff->allocation.bufferSize;                    \
        for(s=d=space; s<maxs; s++) {                         \
            command                                           \
        }                                                     \
        buff->allocation.bufferSize = d - space;                       \
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

static void editorApplyCrLfCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplyCrLfConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplyCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_CR_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplyUtf8CrLfCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyUtf8CrLfConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyUtf8CrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyUtf8Conversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_UTF8_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplyEucCrLfCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyEucCrLfConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyEucCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplyEucConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_EUC_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplySjisCrLfCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplySjisCrLfConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_LF_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplySjisCrConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_CR_CONVERSION(s,d)
                else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                         });
}
static void editorApplySjisConversion(EditorBuffer *buff) {
    EDITOR_ENCODING_WALK_THROUGH_BUFFER(buff, {
            EDITOR_ENCODING_SJIS_CONVERSION(s,d)
            else EDITOR_ENCODING_ELSE_BRANCH(s,d)
                     });
}
static void editorApplyUtf16Conversion(EditorBuffer *buff) {
    unsigned char  *s, *d, *maxs;
    unsigned int cb, cb2;
    int little_endian;
    unsigned char *space;

    space = (unsigned char *)buff->allocation.text;
    maxs = space + buff->allocation.bufferSize;
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
    buff->allocation.bufferSize = d - space;
}

static bool editorBufferStartsWithUtf16Bom(EditorBuffer *buff) {
    unsigned char *s;
    unsigned cb;

    s = (unsigned char *)buff->allocation.text;
    if (buff->allocation.bufferSize >= 2) {
        cb = (*s << 8) + *(s+1);
        if (cb == 0xfeff || cb == 0xfffe)
            return true;
    }
    return false;
}

static void editorPerformSimpleLineFeedConversion(EditorBuffer *buff) {
    if ((options.eolConversion&CR_LF_EOL_CONVERSION)
        && (options.eolConversion & CR_EOL_CONVERSION)) {
        editorApplyCrLfCrConversion(buff);
    } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
        editorApplyCrLfConversion(buff);
    } else if (options.eolConversion & CR_EOL_CONVERSION) {
        editorApplyCrConversion(buff);
    }
}

static void editorPerformEncodingAdjustemets(EditorBuffer *buff) {
    // do different loops for efficiency reasons
    if (options.fileEncoding == MULE_EUROPEAN) {
        editorPerformSimpleLineFeedConversion(buff);
    } else if (options.fileEncoding == MULE_EUC) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            editorApplyEucCrLfCrConversion(buff);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            editorApplyEucCrLfConversion(buff);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            editorApplyEucCrConversion(buff);
        } else {
            editorApplyEucConversion(buff);
        }
    } else if (options.fileEncoding == MULE_SJIS) {
        if ((options.eolConversion&CR_LF_EOL_CONVERSION)
            && (options.eolConversion & CR_EOL_CONVERSION)) {
            editorApplySjisCrLfCrConversion(buff);
        } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
            editorApplySjisCrLfConversion(buff);
        } else if (options.eolConversion & CR_EOL_CONVERSION) {
            editorApplySjisCrConversion(buff);
        } else {
            editorApplySjisConversion(buff);
        }
    } else {
        // default == utf
        if ((options.fileEncoding != MULE_UTF_8 && editorBufferStartsWithUtf16Bom(buff))
            || options.fileEncoding == MULE_UTF_16
            || options.fileEncoding == MULE_UTF_16LE
            || options.fileEncoding == MULE_UTF_16BE
            ) {
            editorApplyUtf16Conversion(buff);
            editorPerformSimpleLineFeedConversion(buff);
        } else {
            // utf-8
            if ((options.eolConversion&CR_LF_EOL_CONVERSION)
                && (options.eolConversion & CR_EOL_CONVERSION)) {
                editorApplyUtf8CrLfCrConversion(buff);
            } else if (options.eolConversion & CR_LF_EOL_CONVERSION) {
                editorApplyUtf8CrLfConversion(buff);
            } else if (options.eolConversion & CR_EOL_CONVERSION) {
                editorApplyUtf8CrConversion(buff);
            } else {
                editorApplyUtf8Conversion(buff);
            }
        }
    }
}

EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset, EditorMarker *previous, EditorMarker *next) {
    EditorMarker *editorMarker;

    ED_ALLOC(editorMarker, EditorMarker);
    editorMarker->buffer = buffer;
    editorMarker->offset = offset;
    editorMarker->previous = previous;
    editorMarker->next = next;

    return editorMarker;
}

EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next) {
    EditorRegionList *regionList;

    ED_ALLOC(regionList, EditorRegionList);
    regionList->region.begin = begin;
    regionList->region.end = end;
    regionList->next = next;

    return regionList;
}


//////////////////////////////////////////////////////////////////////////////
static EditorBufferList *editorBufferTablesInit[EDITOR_BUFF_TAB_SIZE];

void editorInit(void) {
    editorBufferTables.tab = editorBufferTablesInit;
    editorBufferTabNoAllocInit(&editorBufferTables, EDITOR_BUFF_TAB_SIZE);
}

int editorFileStatus(char *path, struct stat *statP) {
    EditorBuffer *buffer;

    buffer = editorGetOpenedBuffer(path);
    if (buffer != NULL) {
        if (statP != NULL) {
            *statP = buffer->stat;
            log_trace("returning stat of %s modified at %s", path, ctime(&buffer->stat.st_mtime));
        }
        return 0;
    }
    return fileStatus(path, statP);
}

time_t editorFileModificationTime(char *path) {
    EditorBuffer *buffer;

    buffer = editorGetOpenedBuffer(path);
    if (buffer != NULL)
        return buffer->stat.st_mtime;
    return fileModificationTime(path);
}

size_t editorFileSize(char *path) {
    EditorBuffer *buffer;

    buffer = editorGetOpenedBuffer(path);
    if (buffer != NULL)
        return buffer->stat.st_size;
    return fileSize(path);
}

bool editorFileExists(char *path) {
    EditorBuffer *buffer;

    buffer = editorGetOpenedBuffer(path);
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
    return(! editorMarkerLess(m2, m1));
}

bool editorMarkerGreater(EditorMarker *m1, EditorMarker *m2) {
    return(editorMarkerLess(m2, m1));
}

bool editorMarkerGreaterOrEq(EditorMarker *m1, EditorMarker *m2) {
    return(editorMarkerLessOrEq(m2, m1));
}

bool editorMarkerListLess(EditorMarkerList *l1, EditorMarkerList *l2) {
    return(editorMarkerLess(l1->marker, l2->marker));
}

bool editorRegionListLess(EditorRegionList *l1, EditorRegionList *l2) {
    if (editorMarkerLess(l1->region.begin, l2->region.begin)) return true;
    if (editorMarkerLess(l2->region.begin, l1->region.begin)) return false;
    // region beginnings are equal, check end
    if (editorMarkerLess(l1->region.end, l2->region.end)) return true;
    if (editorMarkerLess(l2->region.end, l1->region.end)) return false;
    return false;
}

static void editorAffectMarkerToBuffer(EditorBuffer *buffer, EditorMarker *marker) {
    marker->buffer = buffer;
    marker->next = buffer->markers;
    buffer->markers = marker;
    marker->previous = NULL;
    if (marker->next!=NULL)
        marker->next->previous = marker;
}

EditorMarker *editorCreateNewMarker(EditorBuffer *buffer, int offset) {
    EditorMarker *marker;

    ED_ALLOC(marker, EditorMarker);
    *marker = (EditorMarker){.buffer = NULL, .offset = offset, .previous = NULL, .next = NULL};
    editorAffectMarkerToBuffer(buffer, marker);
    return marker;
}

EditorMarker *editorCreateNewMarkerForPosition(Position *position) {
    EditorBuffer *buffer;
    EditorMarker *marker;

    if (position->file==noFileIndex || position->file<0) {
        errorMessage(ERR_INTERNAL, "[editor] creating marker for nonexistant position");
    }
    buffer = editorFindFile(getFileItem(position->file)->name);
    marker = editorCreateNewMarker(buffer, 0);
    editorMoveMarkerToLineCol(marker, position->line, position->col);
    return marker;
}

EditorMarker *editorDuplicateMarker(EditorMarker *marker) {
    return(editorCreateNewMarker(marker->buffer, marker->offset));
}

static void editorRemoveMarkerFromBufferNoFreeing(EditorMarker *marker) {
    if (marker == NULL) return;
    if (marker->next!=NULL) marker->next->previous = marker->previous;
    if (marker->previous==NULL) {
        marker->buffer->markers = marker->next;
    } else {
        marker->previous->next = marker->next;
    }
}

void editorFreeMarker(EditorMarker *marker) {
    if (marker == NULL) return;
    editorRemoveMarkerFromBufferNoFreeing(marker);
    ED_FREE(marker, sizeof(EditorMarker));
}

static void editorFreeTextSpace(char *space, int index) {
    S_editorMemoryBlock *sp;
    sp = (S_editorMemoryBlock *) space;
    sp->next = s_editorMemory[index];
    s_editorMemory[index] = sp;
}

static void editorFreeBuffer(EditorBufferList *list) {
    EditorMarker *m, *mm;
    //&fprintf(stderr,"freeing buffer %s==%s\n", list->buffer->name, list->buffer->fileName);
    if (list->buffer->fileName != list->buffer->name) {
        ED_FREE(list->buffer->fileName, strlen(list->buffer->fileName)+1);
    }
    ED_FREE(list->buffer->name, strlen(list->buffer->name)+1);
    for(m=list->buffer->markers; m!=NULL;) {
        mm = m->next;
        ED_FREE(m, sizeof(EditorMarker));
        m = mm;
    }
    if (list->buffer->bits.textLoaded) {
        //&fprintf(dumpOut,"freeing %d of size %d\n",list->buffer->allocation.allocatedBlock,list->buffer->allocation.allocatedSize);
        // check for magic
        assert(list->buffer->allocation.allocatedBlock[list->buffer->allocation.allocatedSize] == 0x3b);
        editorFreeTextSpace(list->buffer->allocation.allocatedBlock,list->buffer->allocation.allocatedIndex);
    }
    ED_FREE(list->buffer, sizeof(EditorBuffer));
    ED_FREE(list, sizeof(EditorBufferList));
}

static void editorLoadFileIntoBufferText(EditorBuffer *buffer, struct stat *stat) {
    char *space, *fname;
    FILE *file;
    char *bb;
    int n, ss, size;

    fname = buffer->fileName;
    space = buffer->allocation.text;
    size = buffer->allocation.bufferSize;
#if defined (__WIN32__)
    file = openFile(fname, "r");         // was rb, but did not work
#else
    file = openFile(fname, "r");
#endif
    //&fprintf(dumpOut,":loading file %s==%s size %d\n", fname, buffer->name, size);
    if (file == NULL) {
        fatalError(ERR_CANT_OPEN, fname, XREF_EXIT_ERR);
    }
    bb = space; ss = size;
    assert(bb != NULL);
    do {
        n = readFile(bb, 1, ss, file);
        bb = bb + n;
        ss = ss - n;
    } while (n>0);
    closeFile(file);
    if (ss!=0) {
        // this is possible, due to <CR><LF> conversion under MS-DOS
        buffer->allocation.bufferSize -= ss;
        if (ss < 0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"File %s: readed %d chars of %d", fname, size-ss, size);
            editorError(ERR_INTERNAL, tmpBuff);
        }
    }
    editorPerformEncodingAdjustemets(buffer);
    buffer->stat = *stat;
    buffer->bits.textLoaded = true;
}

static void allocNewEditorBufferTextSpace(EditorBuffer *ff, int size) {
    int minSize, allocIndex, allocSize;
    char *space;
    minSize = size + EDITOR_ALLOCATION_RESERVE + EDITOR_FREE_PREFIX_SIZE;
    allocIndex = 11; allocSize = 2048;
    for(; allocSize<minSize; ) {
        allocIndex ++;
        allocSize = allocSize << 1;
    }
    space = (char *)s_editorMemory[allocIndex];
    if (space == NULL) {
        space = malloc(allocSize+1);
        if (space == NULL) fatalError(ERR_NO_MEMORY, "global malloc", XREF_EXIT_ERR);
        // put magic
        space[allocSize] = 0x3b;
    } else {
        s_editorMemory[allocIndex] = s_editorMemory[allocIndex]->next;
    }
    ff->allocation = (EditorBufferAllocationData){.bufferSize = size, .text = space+EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedFreePrefixSize = EDITOR_FREE_PREFIX_SIZE,
                                           .allocatedBlock = space, .allocatedIndex = allocIndex,
                                           .allocatedSize = allocSize};
}

static void fillEmptyEditorBuffer(EditorBuffer *ff, char *aname, int ftnum,
                                  char*afname) {
    ff->bits = (EditorBufferBits){.textLoaded = 0, .modified = 0, .modifiedSinceLastQuasiSave = 0};
    ff->allocation = (EditorBufferAllocationData){.bufferSize = 0, .text = NULL, .allocatedFreePrefixSize = 0,
                                           .allocatedBlock = NULL, .allocatedIndex = 0, .allocatedSize = 0};
    *ff = (EditorBuffer){.name = aname, .ftnum = ftnum, .fileName = afname, .markers = NULL,
                           .allocation = ff->allocation, .bits = ff->bits};
    memset(&ff->stat, 0, sizeof(ff->stat));
}

static EditorBuffer *editorCreateNewBuffer(char *name, char *fileName, struct stat *st) {
    char *allocatedName, *normalizedName, *afname, *normalizedFileName;
    EditorBuffer *buffer;
    EditorBufferList *bufferList;

    normalizedName = normalizeFileName(name, cwd);
    ED_ALLOCC(allocatedName, strlen(normalizedName)+1, char);
    strcpy(allocatedName, normalizedName);
    normalizedFileName = normalizeFileName(fileName, cwd);
    if (strcmp(normalizedFileName, allocatedName)==0) {
        afname = allocatedName;
    } else {
        ED_ALLOCC(afname, strlen(normalizedFileName)+1, char);
        strcpy(afname, normalizedFileName);
    }
    ED_ALLOC(buffer, EditorBuffer);
    fillEmptyEditorBuffer(buffer, allocatedName, 0, afname);
    buffer->stat = *st;

    ED_ALLOC(bufferList, EditorBufferList);
    *bufferList = (EditorBufferList){.buffer = buffer, .next = NULL};
    log_trace("creating buffer '%s' for '%s'", buffer->name, buffer->fileName);

    editorBufferTabAdd(&editorBufferTables, bufferList);

    // set ftnum at the end, because, addfiletabitem calls back the statb
    // from editor, so be tip-top at this moment!
    buffer->ftnum = addFileNameToFileTable(allocatedName);

    return buffer;
}

static void editorSetBufferModifiedFlag(EditorBuffer *buff) {
    buff->bits.modified = true;
    buff->bits.modifiedSinceLastQuasiSave = true;
}

EditorBuffer *editorGetOpenedBuffer(char *name) {
    EditorBuffer editorBuffer;
    EditorBufferList editorBufferList, *element;

    fillEmptyEditorBuffer(&editorBuffer, name, 0, name);
    editorBufferList = (EditorBufferList){.buffer = &editorBuffer, .next = NULL};
    if (editorBufferTabIsMember(&editorBufferTables, &editorBufferList, NULL, &element)) {
        return element->buffer;
    }
    return NULL;
}

EditorBuffer *editorGetOpenedAndLoadedBuffer(char *name) {
    EditorBuffer *res;
    res = editorGetOpenedBuffer(name);
    if (res!=NULL && res->bits.textLoaded) return(res);
    return(NULL);
}

static EditorUndo *newEditorUndoReplace(EditorBuffer *buffer, unsigned offset, unsigned size,
                                          unsigned length, char *str, struct editorUndo *next) {
    EditorUndo *undo;

    ED_ALLOC(undo, EditorUndo);
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

    ED_ALLOC(undo, EditorUndo);
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

    ED_ALLOC(undo, EditorUndo);
    undo->buffer = buffer;
    undo->operation = UNDO_MOVE_BLOCK;
    undo->u.moveBlock.offset = offset;
    undo->u.moveBlock.size = size;
    undo->u.moveBlock.dbuffer = dbuffer;
    undo->u.moveBlock.doffset = doffset;
    undo->next = next;

    return undo;
}

void editorRenameBuffer(EditorBuffer *buff, char *nName, EditorUndo **undo) {
    char newName[MAX_FILE_NAME_SIZE];
    int fileIndex, deleted;
    EditorBuffer dd, *removed;
    EditorBufferList ddl, *memb, *memb2;
    char *oldName;

    strcpy(newName, normalizeFileName(nName, cwd));
    //&sprintf(tmpBuff, "Renaming %s (at %d) to %s (at %d)", buff->name, buff->name, newName, newName);warningMessage(ERR_INTERNAL, tmpBuff);
    fillEmptyEditorBuffer(&dd, buff->name, 0, buff->name);
    ddl = (EditorBufferList){.buffer = &dd, .next = NULL};
    if (!editorBufferTabIsMember(&editorBufferTables, &ddl, NULL, &memb)) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Trying to rename non existing buffer %s", buff->name);
        errorMessage(ERR_INTERNAL, tmpBuff);
        return;
    }
    assert(memb->buffer == buff);
    deleted = editorBufferTabDeleteExact(&editorBufferTables, memb);
    assert(deleted);
    oldName = buff->name;
    ED_ALLOCC(buff->name, strlen(newName)+1, char);
    strcpy(buff->name, newName);
    // update also ftnum
    fileIndex = addFileNameToFileTable(newName);
    getFileItem(fileIndex)->b.commandLineEntered = getFileItem(buff->ftnum)->b.commandLineEntered;
    buff->ftnum = fileIndex;

    *memb = (EditorBufferList){.buffer = buff, .next = NULL};
    if (editorBufferTabIsMember(&editorBufferTables, memb, NULL, &memb2)) {
        editorBufferTabDeleteExact(&editorBufferTables, memb2);
        editorFreeBuffer(memb2);
    }
    editorBufferTabAdd(&editorBufferTables, memb);

    // note undo operation
    if (undo!=NULL) {
        *undo = newEditorUndoRename(buff, oldName, *undo);
    }
    editorSetBufferModifiedFlag(buff);

    // finally create a buffer with old name and empty text in order
    // to keep information that the file is no longer existing
    // so old references will be removed on update (fixing problem of
    // of moving a package into an existing package).
    removed = editorCreateNewBuffer(oldName, oldName, &buff->stat);
    allocNewEditorBufferTextSpace(removed, 0);
    removed->bits.textLoaded = true;
    editorSetBufferModifiedFlag(removed);
}

EditorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName) {
    EditorBuffer  *res;
    struct stat     st;
    res = editorGetOpenedBuffer(name);
    if (res != NULL) {
        return(res);
    }
    fileStatus(fileName, &st);
    res = editorCreateNewBuffer(name, fileName, &st);
    return res;
}

EditorBuffer *editorFindFile(char *name) {
    int size;
    struct stat st;
    EditorBuffer *editorBuffer;

    editorBuffer = editorGetOpenedAndLoadedBuffer(name);
    if (editorBuffer==NULL) {
        editorBuffer = editorGetOpenedBuffer(name);
        if (editorBuffer == NULL) {
            if (fileStatus(name, &st)==0 && (st.st_mode & S_IFMT)!=S_IFDIR) {
                editorBuffer = editorCreateNewBuffer(name, name, &st);
            }
        }
        if (editorBuffer != NULL && fileStatus(editorBuffer->fileName, &st)==0 && (st.st_mode & S_IFMT)!=S_IFDIR) {
            // O.K. supposing that I have a regular file
            size = st.st_size;
            allocNewEditorBufferTextSpace(editorBuffer, size);
            editorLoadFileIntoBufferText(editorBuffer, &st);
        } else {
            return NULL;
        }
    }
    return editorBuffer;
}

EditorBuffer *editorFindFileCreate(char *name) {
    EditorBuffer  *res;
    struct stat     st;
    res = editorFindFile(name);
    if (res == NULL) {
        // create new buffer
        st.st_size = 0;
        st.st_mtime = st.st_atime = st.st_ctime = time(NULL);
        st.st_mode = S_IFCHR;
        res = editorCreateNewBuffer(name, name, &st);
        assert(res!=NULL);
        allocNewEditorBufferTextSpace(res, 0);
        res->bits.textLoaded = true;
    }
    return(res);
}

void editorReplaceString(EditorBuffer *buff, int position, int delsize,
                         char *str, int strlength, EditorUndo **undo) {
    int nsize, oldsize, index, undosize, pattractor;
    char *text, *space, *undotext;
    EditorMarker *m;
    EditorUndo *uu;

    assert(position >=0 && position <= buff->allocation.bufferSize);
    assert(delsize >= 0);
    assert(strlength >= 0);
    oldsize = buff->allocation.bufferSize;
    if (delsize+position > oldsize) {
        // deleting over end of buffer,
        // delete only until end of buffer
        delsize = oldsize - position;
        if (options.debug) assert(0);
    }
    //&fprintf(dumpOut,"replacing string in buffer %d (%s)\n", buff, buff->name);
    nsize = oldsize + strlength - delsize;
    // prepare operation
    if (nsize >= buff->allocation.allocatedSize - buff->allocation.allocatedFreePrefixSize) {
        // resize buffer
        //&sprintf(tmpBuff,"resizing %s from %d(%d) to %d\n", buff->name, buff->allocation.bufferSize, buff->allocation.allocatedSize, nsize);ppcGenRecord(PPC_INFORMATION, tmpBuff);fflush(communicationChannel);
        text = buff->allocation.text;
        space = buff->allocation.allocatedBlock; index = buff->allocation.allocatedIndex;
        allocNewEditorBufferTextSpace(buff, nsize);
        memcpy(buff->allocation.text, text, oldsize);
        buff->allocation.bufferSize = oldsize;
        editorFreeTextSpace(space, index);
    }
    assert(nsize < buff->allocation.allocatedSize - buff->allocation.allocatedFreePrefixSize);
    if (undo!=NULL) {
        // note undo information
        undosize = strlength;
        assert(delsize >= 0);
        // O.K. allocate also 0 at the end
        ED_ALLOCC(undotext, delsize+1, char);
        memcpy(undotext, buff->allocation.text+position, delsize);
        undotext[delsize]=0;
        uu = newEditorUndoReplace(buff, position, undosize, delsize, undotext, *undo);
        *undo = uu;
    }
    // edit text
    memmove(buff->allocation.text+position+strlength, buff->allocation.text+position+delsize,
            buff->allocation.bufferSize - position - delsize);
    memcpy(buff->allocation.text+position, str, strlength);
    buff->allocation.bufferSize = buff->allocation.bufferSize - delsize + strlength;
    //&sprintf(tmpBuff,"setting buffersize of  %s to %d\n", buff->name, buff->allocation.bufferSize);ppcGenRecord(PPC_INFORMATION, tmpBuff);fflush(communicationChannel);
    // update markers
    if (delsize > strlength) {
        if (strlength > 0) pattractor = position + strlength - 1;
        else pattractor = position + strlength;
        for(m=buff->markers; m!=NULL; m=m->next) {
            if (m->offset >= position + strlength) {
                if (m->offset < position+delsize) {
                    m->offset = pattractor;
                } else {
                    m->offset = m->offset - delsize + strlength;
                }
            }
        }
    } else {
        for(m=buff->markers; m!=NULL; m=m->next) {
            if (m->offset >= position + delsize) {
                m->offset = m->offset - delsize + strlength;
            }
        }
    }
    editorSetBufferModifiedFlag(buff);
}

void editorMoveBlock(EditorMarker *dest, EditorMarker *src, int size,
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
    editorReplaceString(db, offd, 0, sb->allocation.text+off1, off2-off1, NULL);
    // now copy text
    off1 = src->offset;
    off2 = off1+size;
    editorReplaceString(db, offd, off2-off1, sb->allocation.text+off1, off2-off1, NULL);
    // save target for undo;
    undodoffset = src->offset;
    // move all markers from moved block
    assert(off1 == src->offset);
    assert(off2 == off1+size);
    mm = sb->markers;
    while (mm!=NULL) {
        tmp = mm->next;
        if (mm->offset>=off1 && mm->offset<off2) {
            editorRemoveMarkerFromBufferNoFreeing(mm);
            mm->offset = offd + (mm->offset-off1);
            editorAffectMarkerToBuffer(db, mm);
        }
        mm = tmp;
    }
    // remove the source block
    editorReplaceString(sb, off1, off2-off1, sb->allocation.text+off1, 0, NULL);
    //
    editorSetBufferModifiedFlag(sb);
    editorSetBufferModifiedFlag(db);
    // add the whole operation into undo
    if (undo!=NULL) {
        *undo = newEditorUndoMove(db, src->offset, off2-off1, sb, undodoffset, *undo);
    }
}

void editorDumpBuffer(EditorBuffer *buff) {
    int i;
    for(i=0; i<buff->allocation.bufferSize; i++) {
        putc(buff->allocation.text[i], dumpOut);
    }
}

void editorDumpBuffers(void) {
    int                     i;
    EditorBufferList      *ll;
    fprintf(dumpOut,"[editorDumpBuffers] start\n");
    for(i=0; i<editorBufferTables.size; i++) {
        for(ll=editorBufferTables.tab[i]; ll!=NULL; ll=ll->next) {
            fprintf(dumpOut,"%d : %s==%s, %d\n", i, ll->buffer->name, ll->buffer->fileName,
                    ll->buffer->bits.textLoaded);
        }
    }
    fprintf(dumpOut,"[editorDumpBuffers] end\n");
}

static void editorQuasiSaveBuffer(EditorBuffer *buffer) {
    buffer->bits.modifiedSinceLastQuasiSave = false;
    buffer->stat.st_mtime = time(NULL);  //? why it does not work with 1;
    FileItem *fileItem = getFileItem(buffer->ftnum);
    fileItem->lastModified = buffer->stat.st_mtime;
}

void editorQuasiSaveModifiedBuffers(void) {
    bool saving = false;
    static time_t lastQuazySaveTime = 0;
    time_t currentTime;

    for (int i=0; i<editorBufferTables.size; i++) {
        for (EditorBufferList *ll=editorBufferTables.tab[i]; ll!=NULL; ll=ll->next) {
            if (ll->buffer->bits.modifiedSinceLastQuasiSave) {
                saving = true;
                goto cont;
            }
        }
    }
 cont:
    if (saving) {
        // sychronization, since last quazi save, there must
        // be at least one second, otherwise times will be wrong
        currentTime = time(NULL);
        if (lastQuazySaveTime > currentTime+5) {
            fatalError(ERR_INTERNAL, "last save in the future, travelling in time?", XREF_EXIT_ERR);
        } else if (lastQuazySaveTime >= currentTime) {
            sleep(1+lastQuazySaveTime-currentTime);
            //&         ppcGenRecord(PPC_INFORMATION,"slept");
        }
    }
    for (int i=0; i<editorBufferTables.size; i++) {
        for (EditorBufferList *ll=editorBufferTables.tab[i]; ll!=NULL; ll=ll->next) {
            if (ll->buffer->bits.modifiedSinceLastQuasiSave) {
                editorQuasiSaveBuffer(ll->buffer);
            }
        }
    }
    lastQuazySaveTime = time(NULL);
}

void editorLoadAllOpenedBufferFiles(void) {
    int i, size;
    EditorBufferList *ll;
    struct stat st;

    for(i=0; i<editorBufferTables.size; i++) {
        for(ll=editorBufferTables.tab[i]; ll!=NULL; ll=ll->next) {
            if (!ll->buffer->bits.textLoaded) {
                if (fileStatus(ll->buffer->fileName, &st)==0) {
                    size = st.st_size;
                    allocNewEditorBufferTextSpace(ll->buffer, size);
                    editorLoadFileIntoBufferText(ll->buffer, &st);
                    //&sprintf(tmpBuff,"preloading %s into %s\n", ll->buffer->fileName, ll->buffer->name); ppcGenRecord(PPC_IGNORE, tmpBuff);
                }
            }
        }
    }
}

int editorRunWithMarkerUntil(EditorMarker *m, int (*until)(int), int step) {
    int     offset, max;
    char    *text;
    assert(step==-1 || step==1);
    offset = m->offset;
    max = m->buffer->allocation.bufferSize;
    text = m->buffer->allocation.text;
    while (offset>=0 && offset<max && (*until)(text[offset])==0) offset += step;
    m->offset = offset;
    if (offset<0) {
        m->offset = 0;
        return(0);
    }
    if (offset>=max) {
        m->offset = max-1;
        return(0);
    }
    return(1);
}

int editorCountLinesBetweenMarkers(EditorMarker *m1, EditorMarker *m2) {
    int     i, max, count;
    char    *text;
    // this can happen after an error in moving, just pass in this case
    if (m1 == NULL || m2==NULL) return(0);
    assert(m1->buffer == m2->buffer);
    assert(m1->offset <= m2->offset);
    text = m1->buffer->allocation.text;
    max = m2->offset;
    count = 0;
    for(i=m1->offset; i<max; i++) {
        if (text[i]=='\n') count ++;
    }
    return(count);
}

static int isNewLine(int c) {return(c=='\n');}
int editorMoveMarkerToNewline(EditorMarker *m, int direction) {
    return(editorRunWithMarkerUntil(m, isNewLine, direction));
}

static int isNonBlank(int c) {return(! isspace(c));}
int editorMoveMarkerToNonBlank(EditorMarker *m, int direction) {
    return(editorRunWithMarkerUntil(m, isNonBlank, direction));
}

static int isNonBlankOrNewline(int c) {return(c=='\n' || ! isspace(c));}
int editorMoveMarkerToNonBlankOrNewline(EditorMarker *m, int direction) {
    return(editorRunWithMarkerUntil(m, isNonBlankOrNewline, direction));
}

static int isNotIdentPart(int c) {return((! isalnum(c)) && c!='_' && c!='$');}
int editorMoveMarkerBeyondIdentifier(EditorMarker *m, int direction) {
    return(editorRunWithMarkerUntil(m, isNotIdentPart, direction));
}

void editorRemoveBlanks(EditorMarker *mm, int direction, EditorUndo **undo) {
    int moffset;

    moffset = mm->offset;
    if (direction < 0) {
        mm->offset --;
        editorMoveMarkerToNonBlank(mm, -1);
        mm->offset ++;
        editorReplaceString(mm->buffer, mm->offset, moffset - mm->offset, "", 0, undo);
    } else if (direction > 0) {
        editorMoveMarkerToNonBlank(mm, 1);
        editorReplaceString(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);
    } else {
        // both directions
        mm->offset --;
        editorMoveMarkerToNonBlank(mm, -1);
        mm->offset ++;
        moffset = mm->offset;
        editorMoveMarkerToNonBlank(mm, 1);
        editorReplaceString(mm->buffer, moffset, mm->offset - moffset, "", 0, undo);
    }
}

void editorMoveMarkerToLineCol(EditorMarker *m, int line, int col) {
    char   *s, *smax;
    int    ln;
    EditorBuffer  *buff;
    int             c;

    assert(m);
    buff = m->buffer;
    s = buff->allocation.text;
    smax = s + buff->allocation.bufferSize;
    ln = 1;
    if (line > 1) {
        for(; s<smax; s++) {
            if (*s == '\n') {
                ln ++;
                if (ln == line) break;
            }
        }
        if (s < smax) s++;
    }
    c = 0;
    for(; s<smax && c<col; s++,c++) {
        if (*s == '\n') break;
    }
    m->offset = s - buff->allocation.text;
    assert(m->offset>=0 && m->offset<=buff->allocation.bufferSize);
}

EditorMarkerList *editorReferencesToMarkers(Reference *refs,
                                              int (*filter)(Reference *, void *),
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
        while (r!=NULL && ! filter(r,filterParam)) r = r->next;
        if (r != NULL) {
            file = r->position.file;
            line = r->position.line;
            col = r->position.col;
            FileItem *fileItem = getFileItem(file);
            buff = editorFindFile(fileItem->name);
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
                        m = editorCreateNewMarker(buff, s - buff->allocation.text);
                        ED_ALLOC(rrr, EditorMarkerList);
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
                    m = editorCreateNewMarker(buff, maxoffset);
                    ED_ALLOC(rrr, EditorMarkerList);
                    *rrr = (EditorMarkerList){.marker = m, .usage = r->usage, .next = res};
                    res = rrr;
                    r = r->next;
                    while (r!=NULL && ! filter(r,filterParam)) r = r->next;
                }
            }
        }
    }
    // get markers in the same order as were references
    // ?? is this still needed?
    LIST_REVERSE(EditorMarkerList, res);
    return(res);
}

Reference *editorMarkersToReferences(EditorMarkerList **mms) {
    EditorMarkerList  *mm;
    EditorBuffer      *buf;
    char                *s, *smax, *off;
    int                 line, col;
    Reference         *res, *rr;
    LIST_MERGE_SORT(EditorMarkerList, *mms, editorMarkerListLess);
    res = NULL;
    mm = *mms;
    while (mm!=NULL) {
        buf = mm->marker->buffer;
        s = buf->allocation.text;
        smax = s + buf->allocation.bufferSize;
        off = buf->allocation.text + mm->marker->offset;
        line = 1; col = 0;
        for( ; s<smax; s++, col++) {
            if (s == off) {
                rr = olcx_alloc(sizeof(Reference));
                rr->position = makePosition(buf->ftnum, line, col);
                fillReference(rr, mm->usage, rr->position, res);
                res = rr;
                mm = mm->next;
                if (mm==NULL || mm->marker->buffer != buf) break;
                off = buf->allocation.text + mm->marker->offset;
            }
            if (*s=='\n') {line++; col = -1;}
        }
        while (mm!=NULL && mm->marker->buffer==buf) {
            rr = olcx_alloc(sizeof(Reference));
            rr->position = makePosition(buf->ftnum, line, 0);
            fillReference(rr, mm->usage, rr->position, res);
            res = rr;
            mm = mm->next;
        }
    }
    LIST_MERGE_SORT(Reference, res, olcxReferenceInternalLessFunction);
    return(res);
}

void editorFreeRegionListNotMarkers(EditorRegionList *occs) {
    EditorRegionList  *o, *next;
    for(o=occs; o!=NULL; ) {
        next = o->next;
        ED_FREE(o, sizeof(EditorRegionList));
        o = next;
    }
}

void editorFreeMarkersAndRegionList(EditorRegionList *occs) {
    EditorRegionList  *o, *next;
    for(o=occs; o!=NULL; ) {
        next = o->next;
        editorFreeMarker(o->region.begin);
        editorFreeMarker(o->region.end);
        ED_FREE(o, sizeof(EditorRegionList));
        o = next;
    }
}

void editorFreeMarkerListNotMarkers(EditorMarkerList *occs) {
    EditorMarkerList  *o, *next;
    for(o=occs; o!=NULL; ) {
        next = o->next;
        ED_FREE(o, sizeof(EditorMarkerList));
        o = next;
    }
}

void editorFreeMarkersAndMarkerList(EditorMarkerList *occs) {
    EditorMarkerList  *o, *next;
    for(o=occs; o!=NULL; ) {
        next = o->next;
        editorFreeMarker(o->marker);
        ED_FREE(o, sizeof(EditorMarkerList));
        o = next;
    }
}

void editorDumpMarker(EditorMarker *mm) {
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->buffer->name), mm->offset, CHAR_ON_MARKER(mm));
    ppcBottomInformation(tmpBuff);
}

void editorDumpMarkerList(EditorMarkerList *mml) {
    EditorMarkerList *mm;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "------------------[[dumping editor markers]]");
    ppcBottomInformation(tmpBuff);
    for(mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->marker == NULL) {
            sprintf(tmpBuff, "[null]");
            ppcBottomInformation(tmpBuff);
        } else {
            sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->marker->buffer->name), mm->marker->offset, CHAR_ON_MARKER(mm->marker));
            ppcBottomInformation(tmpBuff);
        }
    }
    sprintf(tmpBuff, "------------------[[dumpend]]\n");ppcBottomInformation(tmpBuff);
}

void editorDumpRegionList(EditorRegionList *mml) {
    EditorRegionList *mm;
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff,"-------------------[[dumping editor regions]]\n");
    fprintf(dumpOut,"%s\n",tmpBuff);
    //ppcGenTmpBuff();
    for(mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->region.begin == NULL || mm->region.end == NULL) {
            sprintf(tmpBuff,"%ld: [null]", (unsigned long)mm);
            fprintf(dumpOut,"%s\n",tmpBuff);
            //ppcGenTmpBuff();
        } else {
            sprintf(tmpBuff, "%ld: [%s: %d - %d] --> %c - %c", (unsigned long)mm,
                    simpleFileName(mm->region.begin->buffer->name), mm->region.begin->offset,
                    mm->region.end->offset, CHAR_ON_MARKER(mm->region.begin), CHAR_ON_MARKER(mm->region.end));
            fprintf(dumpOut,"%s\n",tmpBuff);
            //ppcGenTmpBuff();
        }
    }
    sprintf(tmpBuff, "------------------[[dumpend]]\n");
    fprintf(dumpOut,"%s\n",tmpBuff);
    //ppcGenTmpBuff();
}

void editorDumpUndoList(EditorUndo *uu) {
    char tmpBuff[TMP_BUFF_SIZE];

    fprintf(dumpOut,"\n\n[undodump] begin\n");
    while (uu!=NULL) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            sprintf(tmpBuff,"replace string [%s:%d] %d (%ld)%s %d", uu->buffer->name,
                    uu->u.replace.offset, uu->u.replace.size, (unsigned long)uu->u.replace.str,
                    uu->u.replace.str, uu->u.replace.strlen);
            if (strlen(uu->u.replace.str)!=uu->u.replace.strlen)
                fprintf(dumpOut,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            fprintf(dumpOut,"%s\n",tmpBuff);
            break;
        case UNDO_RENAME_BUFFER:
            sprintf(tmpBuff,"rename buffer %s %s", uu->buffer->name, uu->u.rename.name);
            fprintf(dumpOut,"%s\n",tmpBuff);
            break;
        case UNDO_MOVE_BLOCK:
            sprintf(tmpBuff,"move block [%s:%d] [%s:%d] size==%d", uu->buffer->name, uu->u.moveBlock.offset, uu->u.moveBlock.dbuffer->name, uu->u.moveBlock.doffset, uu->u.moveBlock.size);
            fprintf(dumpOut,"%s\n",tmpBuff);
            break;
        default:
            errorMessage(ERR_INTERNAL,"Unknown operation to undo");
        }
        uu = uu->next;
    }
    fprintf(dumpOut,"[undodump] end\n");
    fflush(dumpOut);
}


void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2,
                              EditorMarkerList **diff1, EditorMarkerList **diff2) {
    EditorMarkerList *l1, *l2, *ll;
    EditorMarker *m;
    LIST_MERGE_SORT(EditorMarkerList, *list1, editorMarkerListLess);
    LIST_MERGE_SORT(EditorMarkerList, *list2, editorMarkerListLess);
    *diff1 = *diff2 = NULL;
    for(l1 = *list1, l2 = *list2; l1!=NULL && l2!=NULL; ) {
        if (editorMarkerListLess(l1, l2)) {
            m = editorCreateNewMarker(l1->marker->buffer, l1->marker->offset);
            ED_ALLOC(ll, EditorMarkerList);
            *ll = (EditorMarkerList){.marker = m, .usage = l1->usage, .next = *diff1};
            *diff1 = ll;
            l1 = l1->next;
        } else if (editorMarkerListLess(l2, l1)) {
            m = editorCreateNewMarker(l2->marker->buffer, l2->marker->offset);
            ED_ALLOC(ll, EditorMarkerList);
            *ll = (EditorMarkerList){.marker = m, .usage = l2->usage, .next = *diff2};
            *diff2 = ll;
            l2 = l2->next;
        } else {
            l1 = l1->next; l2 = l2->next;
        }
    }
    while (l1 != NULL) {
        m = editorCreateNewMarker(l1->marker->buffer, l1->marker->offset);
        ED_ALLOC(ll, EditorMarkerList);
        *ll = (EditorMarkerList){.marker = m, .usage = l1->usage, .next = *diff1};
        *diff1 = ll;
        l1 = l1->next;
    }
    while (l2 != NULL) {
        m = editorCreateNewMarker(l2->marker->buffer, l2->marker->offset);
        ED_ALLOC(ll, EditorMarkerList);
        *ll = (EditorMarkerList){.marker = m, .usage = l2->usage, .next = *diff2};
        *diff2 = ll;
        l2 = l2->next;
    }
}

void editorSortRegionsAndRemoveOverlaps(EditorRegionList **regions) {
    EditorRegionList  *rr, *rrr;
    EditorMarker      *newend;
    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListLess);
    for(rr= *regions; rr!=NULL; rr=rr->next) {
    contin:
        rrr = rr->next;
        if (rrr!=NULL && rr->region.begin->buffer==rrr->region.begin->buffer) {
            assert(rr->region.begin->buffer == rr->region.end->buffer);  // region consistency check
            assert(rrr->region.begin->buffer == rrr->region.end->buffer);  // region consistency check
            assert(rr->region.begin->offset <= rrr->region.begin->offset);
            newend = NULL;
            if (rrr->region.end->offset <= rr->region.end->offset) {
                // second inside first
                newend = rr->region.end;
                editorFreeMarker(rrr->region.begin);
                editorFreeMarker(rrr->region.end);
            } else if (rrr->region.begin->offset <= rr->region.end->offset) {
                // they have common part
                newend = rrr->region.end;
                editorFreeMarker(rrr->region.begin);
                editorFreeMarker(rr->region.end);
            }
            if (newend!=NULL) {
                rr->region.end = newend;
                rr->next = rrr->next;
                ED_FREE(rrr, sizeof(EditorRegionList));
                rrr = NULL;
                goto contin;
            }
        }
    }
}

void editorSplitMarkersWithRespectToRegions(
                                            EditorMarkerList  **inMarkers,
                                            EditorRegionList  **inRegions,
                                            EditorMarkerList  **outInsiders,
                                            EditorMarkerList  **outOutsiders
                                            ) {
    EditorMarkerList *mm, *nn;
    EditorRegionList *rr;

    *outInsiders = NULL;
    *outOutsiders = NULL;

    LIST_MERGE_SORT(EditorMarkerList, *inMarkers, editorMarkerListLess);
    editorSortRegionsAndRemoveOverlaps(inRegions);

    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *inMarkers);

    //&editorDumpRegionList(*inRegions);
    //&editorDumpMarkerList(*inMarkers);

    rr = *inRegions;
    mm= *inMarkers;
    while (mm!=NULL) {
        nn = mm->next;
        while (rr!=NULL && editorMarkerGreater(rr->region.begin, mm->marker)) rr = rr->next;
        if (rr!=NULL && editorMarkerGreater(rr->region.end, mm->marker)) {
            // is inside
            mm->next = *outInsiders;
            *outInsiders = mm;
        } else {
            // is outside
            mm->next = *outOutsiders;
            *outOutsiders = mm;
        }
        mm = nn;
    }

    *inMarkers = NULL;
    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *outInsiders);
    LIST_REVERSE(EditorMarkerList, *outOutsiders);
    //&editorDumpMarkerList(*outInsiders);
    //&editorDumpMarkerList(*outOutsiders);
}

void editorRestrictMarkersToRegions(EditorMarkerList **mm, EditorRegionList **regions) {
    EditorMarkerList *ins, *outs;
    editorSplitMarkersWithRespectToRegions(mm, regions, &ins, &outs);
    *mm = ins;
    editorFreeMarkersAndMarkerList(outs);
}

EditorMarker *editorCrMarkerForBufferBegin(EditorBuffer *buffer) {
    return(editorCreateNewMarker(buffer,0));
}

EditorMarker *editorCrMarkerForBufferEnd(EditorBuffer *buffer) {
    return(editorCreateNewMarker(buffer,buffer->allocation.bufferSize));
}

EditorRegionList *editorWholeBufferRegion(EditorBuffer *buffer) {
    EditorMarker *bufferBegin, *bufferEnd;
    EditorRegion theBufferRegion;
    EditorRegionList *theBufferRegionList;

    bufferBegin = editorCrMarkerForBufferBegin(buffer);
    bufferEnd = editorCrMarkerForBufferEnd(buffer);
    theBufferRegion = (EditorRegion){.begin = bufferBegin, .end = bufferEnd};
    ED_ALLOC(theBufferRegionList, EditorRegionList);
    *theBufferRegionList = (EditorRegionList){.region = theBufferRegion, .next = NULL};

    return theBufferRegionList;
}

static EditorBufferList *editorComputeAllBuffersList(void) {
    EditorBufferList  *l, *rr, *res;

    res = NULL;
    for (int i=0; i<editorBufferTables.size; i++) {
        for (l=editorBufferTables.tab[i]; l!=NULL; l=l->next) {
            ED_ALLOC(rr, EditorBufferList);
            *rr = (EditorBufferList){.buffer = l->buffer, .next = res};
            res = rr;
        }
    }
    return res;
}

static void editorFreeBufferListButNotBuffers(EditorBufferList *list) {
    EditorBufferList *l, *n;

    l=list;
    while (l!=NULL) {
        n = l->next;
        ED_FREE(l, sizeof(EditorBufferList));
        l = n;
    }
}

static int editorBufferNameLess(EditorBufferList*l1,EditorBufferList*l2) {
    return(strcmp(l1->buffer->name, l2->buffer->name));
}

// TODO, do all this stuff better!
// This is still quadratic on number of opened buffers
// for recursive search
int editorMapOnNonexistantFiles(
                                char *dirname,
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
    bl = editorComputeAllBuffersList();
    LIST_MERGE_SORT(EditorBufferList, bl, editorBufferNameLess);
    ll = bl;
    //&sprintf(tmpBuff, "ENTER!!!"); ppcGenRecord(PPC_IGNORE,tmpBuff);
    while(ll!=NULL) {
        if (fnnCmp(ll->buffer->name, dirname, dlen)==0
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
            // check if file exists, map only nonexistant
            struct stat st;
            if (fileStatus(ll->buffer->name, &st)!=0) {
                // get file name
                //&sprintf(tmpBuff, "MAPPING %s as %s in %s", ll->buffer->name, fname, dirname); ppcGenRecord(PPC_IGNORE,tmpBuff);
                (*fun)(fname, a1, a2, a3, a4, a5);
                res = 1;
                // skip all files in the same directory
                lastMapped = ll->buffer->name;
                lastMappedLen = dlen+1+fnlen;
                ll = ll->next;
                while (ll!=NULL
                       && fnnCmp(ll->buffer->name, lastMapped, lastMappedLen)==0
                       && (ll->buffer->name[lastMappedLen]=='/' || ll->buffer->name[lastMappedLen]=='\\')) {
                    //&sprintf(tmpBuff, "SKIPPING %s", ll->buffer->name); ppcGenRecord(PPC_IGNORE,tmpBuff);
                    ll = ll->next;
                }
            } else {
                ll = ll->next;
            }
        } else {
            ll = ll->next;
        }
    }
    editorFreeBufferListButNotBuffers(bl);
    //&sprintf(tmpBuff, "QUIT!!!"); ppcGenRecord(PPC_IGNORE,tmpBuff);
    return(res);
}

static void editorCloseBuffer(EditorBufferList *memb, int index) {
    EditorBufferList **ll;
    //&sprintf(tmpBuff,"closing buffer %s %s\n", memb->buffer->name, memb->buffer->fileName);ppcGenRecord(PPC_IGNORE, tmpBuff);
    for (ll = &editorBufferTables.tab[index]; (*ll)!=NULL; ll = &(*ll)->next) {
        if (*ll == memb) break;
    }
    if (*ll == memb) {
        // O.K. now, free the buffer
        *ll = (*ll)->next;
        editorFreeBuffer(memb);
    }
}

#define BUFFER_IS_CLOSABLE(buffer) (                                    \
                                    buffer->bits.textLoaded                \
                                    && buffer->markers==NULL            \
                                    && buffer->name==buffer->fileName  /* not -preloaded */ \
                                    && ! buffer->bits.modified             \
                                    )

// be very carefull when using this function, because of interpretation
// of 'Closable', this should be additional field: 'closable' or what
void editorCloseBufferIfClosable(char *name) {
    EditorBuffer dd;
    EditorBufferList ddl, *memb;
    int index;

    fillEmptyEditorBuffer(&dd, name, 0, name);
    ddl = (EditorBufferList){.buffer = &dd, .next = NULL};
    if (editorBufferTabIsMember(&editorBufferTables, &ddl, &index, &memb)) {
        if (BUFFER_IS_CLOSABLE(memb->buffer)) {
            editorCloseBuffer(memb, index);
        }
    }
}

void editorCloseAllBuffersIfClosable(void) {
    EditorBufferList *list, *next;

    for (int i=0; i<editorBufferTables.size; i++) {
        for(list=editorBufferTables.tab[i]; list!=NULL;) {
            next = list->next;
            //& fprintf(dumpOut, "closable %d for %s(%d) %s(%d)\n", BUFFER_IS_CLOSABLE(list->buffer), list->buffer->name, list->buffer->name, list->buffer->fileName, list->buffer->fileName);fflush(dumpOut);
            if (BUFFER_IS_CLOSABLE(list->buffer)) editorCloseBuffer(list, i);
            list = next;
        }
    }
}

void editorCloseAllBuffers(void) {
    int i;
    EditorBufferList *ll,*nn;

    for(i=0; i<editorBufferTables.size; i++) {
        for(ll=editorBufferTables.tab[i]; ll!=NULL;) {
            nn = ll->next;
            editorFreeBuffer(ll);
            ll = nn;
        }
        editorBufferTables.tab[i] = NULL;
    }
}
