#ifndef EDITORMARKER_H_INCLUDED
#define EDITORMARKER_H_INCLUDED

#include "editorbuffer.h"
#include "position.h"
#include "usage.h"

typedef struct editorMarker {
    EditorBuffer        *buffer;
    unsigned             offset;   /* Offset in the buffer */
    struct editorMarker *previous; // previous marker in this buffer
    struct editorMarker *next;     // next marker in this buffer
} EditorMarker;

typedef struct editorMarkerList {
    EditorMarker     *marker;
    Usage             usage;
    struct editorMarkerList *next;
} EditorMarkerList;


extern EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset);
extern EditorMarker *newEditorMarkerForPosition(Position position);

extern EditorMarker *duplicateEditorMarker(EditorMarker *marker);

extern void attachMarkerToBuffer(EditorMarker *marker, EditorBuffer *buffer);
extern void moveEditorMarkerToLineAndColumn(EditorMarker *marker, int line, int col);

extern bool editorMarkerBefore(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerAfter(EditorMarker *m1, EditorMarker *m2);
extern bool editorMarkerListBefore(EditorMarkerList *l1, EditorMarkerList *l2);

extern int moveEditorMarkerToNewline(EditorMarker *m, int direction);
extern int moveEditorMarkerToNonBlank(EditorMarker *m, int direction);
extern int moveEditorMarkerBeyondIdentifier(EditorMarker *m, int direction);
extern int moveEditorMarkerToNonBlankOrNewline(EditorMarker *m, int direction);

extern void removeEditorMarkerFromBufferWithoutFreeing(EditorMarker *marker);
extern void freeEditorMarker(EditorMarker *marker);
extern void freeEditorMarkerListButNotMarkers(EditorMarkerList *list);
extern void freeEditorMarkerListAndMarkers(EditorMarkerList *list);

extern void editorDumpMarker(EditorMarker *marker);
extern void editorDumpMarkerList(EditorMarkerList *markerList);

#endif
