#ifndef EDITORBUFFER_H_INCLUDED
#define EDITORBUFFER_H_INCLUDED

#include <time.h>
#include <stdbool.h>


typedef struct {
    int   bufferSize;
    char *text;                 /* Where is the actual buffer? */
    int   allocatedFreePrefixSize; /* How much space have we allocated *before* text? */
    char *allocatedBlock;
    int   allocatedIndex;
    int   allocatedSize;
} EditorBufferAllocationData;

typedef struct editorBuffer {
    char                      *name;
    int                        fileNumber;
    char                      *fileName;
    time_t                     modificationTime;
    bool                       textLoaded : 1;
    bool                       modified : 1;
    bool                       modifiedSinceLastQuasiSave : 1;
    size_t                     size;
    struct editorMarker       *markers;
    EditorBufferAllocationData allocation;
} EditorBuffer;

typedef struct editorBufferList {
    EditorBuffer            *buffer;
    struct editorBufferList *next;
} EditorBufferList;


extern void freeEditorBuffer(EditorBufferList *list);

#endif
