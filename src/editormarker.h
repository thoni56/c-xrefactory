#ifndef EDITORMARKER_H_INCLUDED
#define EDITORMARKER_H_INCLUDED

#include "editorbuffer.h"
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



extern void removeEditorMarkerFromBufferWithoutFreeing(EditorMarker *marker);
extern void freeEditorMarker(EditorMarker *marker);

#endif
