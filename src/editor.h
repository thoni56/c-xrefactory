#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <sys/stat.h>

#include "head.h"
#include "completion.h"
#include "editorbuffer.h"
#include "editormarker.h"
#include "proto.h"
#include "reference.h"
#include "undo.h"


typedef struct editorRegion {
    EditorMarker *begin;
    EditorMarker *end;
} EditorRegion;

typedef struct editorRegionList {
    EditorRegion      region;
    struct editorRegionList *next;
} EditorRegionList;


// EditorBuffer functions...
extern void replaceStringInEditorBuffer(EditorBuffer *buff, int position, int delsize, char *str,
                                        int strlength, EditorUndo **undo);
extern void moveBlockInEditorBuffer(EditorMarker *src, EditorMarker *dest, int size, EditorUndo **undo);
extern void dumpEditorBuffer(EditorBuffer *buffer);
extern void dumpEditorBuffers(void);
extern void quasiSaveModifiedEditorBuffers(void);
extern void loadAllOpenedEditorBuffers(void);
extern void closeEditorBufferIfCloseable(char *name);
extern void closeAllEditorBuffersIfClosable(void);
extern void closeAllEditorBuffers(void);

// Hopefully temporary for extraction of EditorBuffer
extern void freeTextSpace(char *space, int index);
extern void loadFileIntoEditorBuffer(EditorBuffer *buffer, time_t modificationTime, size_t fileSize);
extern void allocateNewEditorBufferTextSpace(EditorBuffer *buffer, int size);


extern void editorInit(void);

extern bool   editorFileExists(char *path);
extern size_t editorFileSize(char *path);
extern time_t editorFileModificationTime(char *path);

extern EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next);


extern EditorMarkerList *convertReferencesToEditorMarkers(Reference *references);
extern Reference        *convertEditorMarkersToReferences(EditorMarkerList **markerList);

extern void          editorDumpRegionList(EditorRegionList *mml);
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
extern void              freeEditorMarkerListAndMarkers(EditorMarkerList *occs);
extern bool editorMapOnNonExistantFiles(char *dirname, void (*fun)(MAP_FUN_SIGNATURE), SearchDepth depth, char *a1,
                                        char *a2, Completions *a3, void *a4, int *a5);

#endif
