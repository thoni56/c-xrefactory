#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include "proto.h"

/* ***************** editor structures ********************** */

typedef enum editorUndoOperations {
    UNDO_REPLACE_STRING,
    UNDO_RENAME_BUFFER,
    UNDO_MOVE_BLOCK,
} EditorUndoOperations;

typedef enum editors {
    EDITOR_UNKNOWN,
    EDITOR_EMACS,
    EDITOR_JEDIT,
} Editors;

typedef struct editorBufferAllocationData {
    int     bufferSize;
    char	*text;
    int		allocatedFreePrefixSize;
    char	*allocatedBlock;
    int		allocatedIndex;
    int		allocatedSize;
} EditorBufferAllocationData;

typedef struct editorBuffer {
    char                             *name;
    int                               fileIndex;
    char                             *fileName;
    time_t                            modificationTime;
    bool                              textLoaded : 1;
    bool                              modified : 1;
    bool                              modifiedSinceLastQuasiSave : 1;
    size_t                            size;
    struct editorMarker              *markers;
    struct editorBufferAllocationData allocation;
} EditorBuffer;

typedef struct editorBufferList {
    struct editorBuffer     *buffer;
    struct editorBufferList	*next;
} EditorBufferList;

typedef struct editorMarker {
    struct editorBuffer		*buffer;
    unsigned                offset;
    struct editorMarker     *previous;      // previous marker in this buffer
    struct editorMarker     *next;          // next marker in this buffer
} EditorMarker;

typedef struct editorMarkerList {
    struct editorMarker		*marker;
    struct usage		usage;
    struct editorMarkerList	*next;
} EditorMarkerList;

typedef struct editorRegion {
    struct editorMarker		*begin;
    struct editorMarker		*end;
} EditorRegion;

typedef struct editorRegionList {
    struct editorRegion		region;
    struct editorRegionList	*next;
} EditorRegionList;

typedef struct editorUndo {
    struct editorBuffer	*buffer;
    enum editorUndoOperations operation;
    union editorUndoUnion {
        struct editorUndoStrReplace {
            unsigned			offset;
            unsigned            size;
            unsigned			strlen;
            char				*str;
        } replace;
        struct editorUndoRenameBuff {
            char				*name;
        } rename;
        struct editorUndoMoveBlock {
            unsigned			offset;
            unsigned			size;
            struct editorBuffer	*dbuffer;
            unsigned			doffset;
        } moveBlock;
    } u;
    struct editorUndo     *next;
} EditorUndo;


extern EditorUndo *editorUndo;


extern void editorInit(void);

extern bool editorFileExists(char *path);
extern size_t editorFileSize(char *path);
extern time_t editorFileModificationTime(char *path);
extern int editorFileStatus(char *path, struct stat *statP);

extern EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset, EditorMarker *previous, EditorMarker *next);
extern bool editorMarkerLess(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerLessOrEq(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerGreater(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerGreaterOrEq(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerListLess(EditorMarkerList *l1, EditorMarkerList *l2);
extern bool editorRegionListLess(EditorRegionList *l1, EditorRegionList *l2);

extern EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next);

extern EditorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName);
extern EditorBuffer *editorGetOpenedBuffer(char *name);
extern EditorBuffer *editorGetOpenedAndLoadedBuffer(char *name);
extern EditorBuffer *editorFindFile(char *name);
extern EditorBuffer *editorFindFileCreate(char *name);
extern EditorMarker *editorCreateNewMarkerForPosition(Position *pos);
extern EditorMarkerList *editorReferencesToMarkers(Reference *refs, bool (*filter)(Reference *, void *), void *filterParam);
extern Reference *editorMarkersToReferences(EditorMarkerList **mms);
extern void editorRenameBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo);
extern void editorReplaceString(EditorBuffer *buff, int position, int delsize, char *str, int strlength, EditorUndo **undo);
extern void editorMoveBlock(EditorMarker *dest, EditorMarker *src, int size, EditorUndo **undo);
extern void editorDumpBuffer(EditorBuffer *buff);
extern void editorDumpBuffers(void);
extern void editorDumpMarker(EditorMarker *mm);
extern void editorDumpMarkerList(EditorMarkerList *mml);
extern void editorDumpRegionList(EditorRegionList *mml);
extern void editorQuasiSaveModifiedBuffers(void);
extern void editorLoadAllOpenedBufferFiles(void);
extern EditorMarker *editorCreateNewMarker(EditorBuffer *buff, int offset);
extern EditorMarker *editorDuplicateMarker(EditorMarker *mm);
extern int editorCountLinesBetweenMarkers(EditorMarker *m1, EditorMarker *m2);
extern int editorRunWithMarkerUntil(EditorMarker *m, int (*until)(int), int step);
extern int editorMoveMarkerToNewline(EditorMarker *m, int direction);
extern int editorMoveMarkerToNonBlank(EditorMarker *m, int direction);
extern int editorMoveMarkerBeyondIdentifier(EditorMarker *m, int direction);
extern int editorMoveMarkerToNonBlankOrNewline(EditorMarker *m, int direction);
extern void editorRemoveBlanks(EditorMarker *mm, int direction, EditorUndo **undo);
extern void editorDumpUndoList(EditorUndo *uu);
extern void editorMoveMarkerToLineCol(EditorMarker *m, int line, int col);
extern void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2,
                                     EditorMarkerList **diff1, EditorMarkerList **diff2);
extern void editorFreeMarker(EditorMarker *marker);
extern void editorFreeMarkerListNotMarkers(EditorMarkerList *occs);
extern void editorFreeMarkersAndRegionList(EditorRegionList *occs);
extern void editorFreeRegionListNotMarkers(EditorRegionList *occs);
extern void editorSortRegionsAndRemoveOverlaps(EditorRegionList **regions);
extern void editorSplitMarkersWithRespectToRegions(EditorMarkerList  **inMarkers,
                                                   EditorRegionList  **inRegions,
                                                   EditorMarkerList  **outInsiders,
                                                   EditorMarkerList  **outOutsiders);
extern void editorRestrictMarkersToRegions(EditorMarkerList **mm, EditorRegionList **regions);
extern EditorMarker *editorCrMarkerForBufferBegin(EditorBuffer *buffer);
extern EditorMarker *editorCrMarkerForBufferEnd(EditorBuffer *buffer);
extern EditorRegionList *editorWholeBufferRegion(EditorBuffer *buffer);
extern void editorFreeMarkersAndMarkerList(EditorMarkerList *occs);
extern int editorMapOnNonexistantFiles(char *dirname,
                                       void (*fun)(MAP_FUN_SIGNATURE),
                                       int depth,
                                       char *a1,
                                       char *a2,
                                       Completions *a3,
                                       void *a4,
                                       int *a5);
extern void editorCloseBufferIfClosable(char *name);
extern void editorCloseAllBuffersIfClosable(void);
extern void editorCloseAllBuffers(void);

#endif
