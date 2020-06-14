#ifndef EDITOR_H
#define EDITOR_H

#include "proto.h"

/* ***************** editor structures ********************** */

enum editorUndoOperations {
    UNDO_REPLACE_STRING,
    UNDO_RENAME_BUFFER,
    UNDO_MOVE_BLOCK,
};

enum editors {
    EDITOR_UNKNOWN,
    EDITOR_EMACS,
    EDITOR_JEDIT,
};

typedef     struct editorBufferAllocationData {
    int     bufferSize;
    char	*text;
    int		allocatedFreePrefixSize;
    char	*allocatedBlock;
    int		allocatedIndex;
    int		allocatedSize;
} S_editorBufferAllocationData;

typedef struct editorBufferBits {
    bool textLoaded:1;
    bool modified:1;
    bool modifiedSinceLastQuasiSave:1;
} S_editorBufferBits;

typedef struct editorBuffer {
    char                *name;
    int					ftnum;
    char				*fileName;
    struct stat			stat;
    struct editorMarker	*markers;
    struct editorBufferAllocationData a;
    struct editorBufferBits b;
} EditorBuffer;

typedef struct editorBufferList {
    struct editorBuffer     *f;
    struct editorBufferList	*next;
} EditorBufferList;

typedef struct editorMarker {
    struct editorBuffer		*buffer;
    unsigned                offset;
    struct editorMarker     *previous;      // previous marker in this buffer
    struct editorMarker     *next;          // next marker in this buffer
} EditorMarker;

typedef struct editorMarkerList {
    struct editorMarker		*d;
    struct usageBits		usage;
    struct editorMarkerList	*next;
} S_editorMarkerList;

typedef struct editorRegion {
    struct editorMarker		*b;
    struct editorMarker		*e;
} S_editorRegion;

typedef struct editorRegionList {
    struct editorRegion		r;
    struct editorRegionList	*next;
} S_editorRegionList;

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
} S_editorUndo;


extern S_editorUndo *s_editorUndo;


extern void editorInit(void);
extern EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset, EditorMarker *previous, EditorMarker *next);
extern S_editorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, S_editorRegionList *next);
extern int statb(char *path, struct stat  *statbuf);
extern int editorMarkerLess(EditorMarker *m1, EditorMarker *m2);
extern int editorMarkerLessOrEq(EditorMarker *m1, EditorMarker *m2);
extern int editorMarkerGreater(EditorMarker *m1, EditorMarker *m2);
extern int editorMarkerGreaterOrEq(EditorMarker *m1, EditorMarker *m2);
extern int editorMarkerListLess(S_editorMarkerList *l1, S_editorMarkerList *l2);
extern int editorRegionListLess(S_editorRegionList *l1, S_editorRegionList *l2);
extern EditorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName);
extern EditorBuffer *editorGetOpenedBuffer(char *name);
extern EditorBuffer *editorGetOpenedAndLoadedBuffer(char *name);
extern EditorBuffer *editorFindFile(char *name);
extern EditorBuffer *editorFindFileCreate(char *name);
extern EditorMarker *editorCrNewMarkerForPosition(Position *pos);
extern S_editorMarkerList *editorReferencesToMarkers(Reference *refs, int (*filter)(Reference *, void *), void *filterParam);
extern Reference *editorMarkersToReferences(S_editorMarkerList **mms);
extern void editorRenameBuffer(EditorBuffer *buff, char *newName, S_editorUndo **undo);
extern void editorReplaceString(EditorBuffer *buff, int position, int delsize, char *str, int strlength, S_editorUndo **undo);
extern void editorMoveBlock(EditorMarker *dest, EditorMarker *src, int size, S_editorUndo **undo);
extern void editorDumpBuffer(EditorBuffer *buff);
extern void editorDumpBuffers(void);
extern void editorDumpMarker(EditorMarker *mm);
extern void editorDumpMarkerList(S_editorMarkerList *mml);
extern void editorDumpRegionList(S_editorRegionList *mml);
extern void editorQuasiSaveModifiedBuffers(void);
extern void editorLoadAllOpenedBufferFiles(void);
extern EditorMarker *editorCrNewMarker(EditorBuffer *buff, int offset);
extern EditorMarker *editorDuplicateMarker(EditorMarker *mm);
extern int editorCountLinesBetweenMarkers(EditorMarker *m1, EditorMarker *m2);
extern int editorRunWithMarkerUntil(EditorMarker *m, int (*until)(int), int step);
extern int editorMoveMarkerToNewline(EditorMarker *m, int direction);
extern int editorMoveMarkerToNonBlank(EditorMarker *m, int direction);
extern int editorMoveMarkerBeyondIdentifier(EditorMarker *m, int direction);
extern int editorMoveMarkerToNonBlankOrNewline(EditorMarker *m, int direction);
extern void editorRemoveBlanks(EditorMarker *mm, int direction, S_editorUndo **undo);
extern void editorDumpUndoList(S_editorUndo *uu);
extern void editorMoveMarkerToLineCol(EditorMarker *m, int line, int col);
extern void editorMarkersDifferences(
                                     S_editorMarkerList **list1, S_editorMarkerList **list2,
                                     S_editorMarkerList **diff1, S_editorMarkerList **diff2);
extern void editorFreeMarker(EditorMarker *marker);
extern void editorFreeMarkerListNotMarkers(S_editorMarkerList *occs);
extern void editorFreeMarkersAndRegionList(S_editorRegionList *occs);
extern void editorFreeRegionListNotMarkers(S_editorRegionList *occs);
extern void editorSortRegionsAndRemoveOverlaps(S_editorRegionList **regions);
extern void editorSplitMarkersWithRespectToRegions(
                                                   S_editorMarkerList  **inMarkers,
                                                   S_editorRegionList  **inRegions,
                                                   S_editorMarkerList  **outInsiders,
                                                   S_editorMarkerList  **outOutsiders
                                                   );
extern void editorRestrictMarkersToRegions(S_editorMarkerList **mm, S_editorRegionList **regions);
extern EditorMarker *editorCrMarkerForBufferBegin(EditorBuffer *buffer);
extern EditorMarker *editorCrMarkerForBufferEnd(EditorBuffer *buffer);
extern S_editorRegionList *editorWholeBufferRegion(EditorBuffer *buffer);
extern void editorFreeMarkersAndMarkerList(S_editorMarkerList *occs);
extern int editorMapOnNonexistantFiles(char *dirname,
                                       void (*fun)(MAP_FUN_SIGNATURE),
                                       int depth,
                                       char *a1,
                                       char *a2,
                                       Completions *a3,
                                       void *a4,
                                       int *a5
                                       );
extern void editorCloseBufferIfClosable(char *name);
extern void editorCloseAllBuffersIfClosable(void);
extern void editorCloseAllBuffers(void);

#endif
