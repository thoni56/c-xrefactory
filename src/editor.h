#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include "editorbuffer.h"
#include "usage.h"

#include "reference.h"
#include "completion.h"



/* ***************** editor structures ********************** */

typedef enum editorUndoOperation {
    UNDO_REPLACE_STRING,
    UNDO_RENAME_BUFFER,
    UNDO_MOVE_BLOCK,
} EditorUndoOperation;

typedef enum {
    EDITOR_UNKNOWN,
    EDITOR_EMACS,
    EDITOR_JEDIT,
} Editors;

typedef struct editorMarker {
    EditorBuffer        *buffer;
    unsigned             offset;
    struct editorMarker *previous; // previous marker in this buffer
    struct editorMarker *next;     // next marker in this buffer
} EditorMarker;

typedef struct editorMarkerList {
    EditorMarker     *marker;
    Usage             usage;
    struct editorMarkerList *next;
} EditorMarkerList;

typedef struct editorRegion {
    EditorMarker *begin;
    EditorMarker *end;
} EditorRegion;

typedef struct editorRegionList {
    EditorRegion      region;
    struct editorRegionList *next;
} EditorRegionList;

typedef struct editorUndo {
    EditorBuffer             *buffer;
    EditorUndoOperation operation;
    union editorUndoUnion {
        struct editorUndoStrReplace {
            unsigned offset;
            unsigned size;
            unsigned strlen;
            char    *str;
        } replace;
        struct editorUndoRenameBuff {
            char *name;
        } rename;
        struct editorUndoMoveBlock {
            unsigned      offset;
            unsigned      size;
            EditorBuffer *dbuffer;
            unsigned      doffset;
        } moveBlock;
    } u;
    struct editorUndo *next;
} EditorUndo;

extern EditorUndo *editorUndo;

extern void editorInit(void);

extern bool   editorFileExists(char *path);
extern size_t editorFileSize(char *path);
extern time_t editorFileModificationTime(char *path);
extern int    editorFileStatus(char *path, struct stat *statP);

extern bool editorMarkerBefore(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerBeforeOrSame(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerAfter(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerListBefore(EditorMarkerList *l1, EditorMarkerList *l2);
extern bool editorRegionListBefore(EditorRegionList *l1, EditorRegionList *l2);

extern EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset);
extern EditorMarker     *newEditorMarkerForPosition(Position *pos);
extern EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next);

extern EditorBuffer     *getOpenedEditorBuffer(char *name);
extern EditorBuffer     *getOpenedAndLoadedEditorBuffer(char *name);
extern void          renameEditorBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo);
extern void          replaceStringInEditorBuffer(EditorBuffer *buff, int position, int delsize,
                                                 char *str, int strlength, EditorUndo **undo);
extern void          moveBlockInEditorBuffer(EditorMarker *dest, EditorMarker *src, int size,
                                             EditorUndo **undo);
extern void          editorDumpBuffer(EditorBuffer *buff);

extern EditorMarkerList *convertReferencesToEditorMarkers(Reference *references);
extern Reference        *convertEditorMarkersToReferences(EditorMarkerList **markerList);

extern void          editorDumpBuffers(void);
extern void          editorDumpMarker(EditorMarker *mm);
extern void          editorDumpMarkerList(EditorMarkerList *mml);
extern void          editorDumpRegionList(EditorRegionList *mml);
extern void          quasiSaveModifiedEditorBuffers(void);
extern void          loadAllOpenedEditorBuffers(void);
extern EditorMarker *duplicateEditorMarker(EditorMarker *mm);
extern int           countLinesBetweenEditorMarkers(EditorMarker *m1, EditorMarker *m2);
extern bool          runWithEditorMarkerUntil(EditorMarker *m, int (*until)(int), int step);
extern int           moveEditorMarkerToNewline(EditorMarker *m, int direction);
extern int           editorMoveMarkerToNonBlank(EditorMarker *m, int direction);
extern int           moveEditorMarkerBeyondIdentifier(EditorMarker *m, int direction);
extern int           moveEditorMarkerToNonBlankOrNewline(EditorMarker *m, int direction);
extern void removeBlanksAtEditorMarker(EditorMarker *mm, int direction, EditorUndo **undo);
extern void          editorDumpUndoList(EditorUndo *uu);
extern void          moveEditorMarkerToLineAndColumn(EditorMarker *m, int line, int col);
extern void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2, EditorMarkerList **diff1,
                                     EditorMarkerList **diff2);
extern void freeEditorMarker(EditorMarker *marker);
extern void freeEditorMarkerListButNotMarkers(EditorMarkerList *occs);
extern void freeEditorMarkersAndRegionList(EditorRegionList *occs);
extern void sortEditorRegionsAndRemoveOverlaps(EditorRegionList **regions);
extern void splitEditorMarkersWithRespectToRegions(EditorMarkerList **inMarkers,
                                                   EditorRegionList **inRegions,
                                                   EditorMarkerList **outInsiders,
                                                   EditorMarkerList **outOutsiders);
extern void restrictEditorMarkersToRegions(EditorMarkerList **mm, EditorRegionList **regions);
extern EditorMarker     *createEditorMarkerForBufferBegin(EditorBuffer *buffer);
extern EditorMarker     *createEditorMarkerForBufferEnd(EditorBuffer *buffer);
extern EditorRegionList *createEditorRegionForWholeBuffer(EditorBuffer *buffer);
extern void              freeEditorMarkersAndMarkerList(EditorMarkerList *occs);
extern int  editorMapOnNonexistantFiles(char *dirname, void (*fun)(MAP_FUN_SIGNATURE), int depth, char *a1,
                                        char *a2, Completions *a3, void *a4, int *a5);
extern void closeEditorBufferIfClosable(char *name);
extern void closeAllEditorBuffersIfClosable(void);
extern void closeAllEditorBuffers(void);

// For extraction of EditorBuffer
extern void freeTextSpace(char *space, int index);
extern void loadFileIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime, size_t fileSize);
extern void allocNewEditorBufferTextSpace(EditorBuffer *buffer, int size);

#endif
