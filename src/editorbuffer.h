#ifndef EDITORBUFFER_H_INCLUDED
#define EDITORBUFFER_H_INCLUDED

#include <time.h>
#include <stdbool.h>

typedef struct editorUndo EditorUndo; /* Mutual dependency... */

typedef struct {
    int   bufferSize;
    char *text;                    /* Where is the actual buffer? */
    int   allocatedFreePrefixSize; /* How much space have we allocated *before* text? */
    char *allocatedBlock;
    int   allocatedIndex;
    int   allocatedSize;
} EditorBufferAllocationData;

typedef struct editorBuffer {
    char                      *fileName;
    int                        fileNumber;
    char                      *preLoadedFromFile;   /* If the content was preloaded this is
                                               the name on disc for the content else
                                               NULL. */
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


extern void freeEditorBuffer(EditorBuffer *buffer);
extern EditorBuffer *newEditorBuffer(char *fileName, int fileNumber, char *preLoadedFromFile,
                                     time_t modificationTime, size_t size);
extern EditorBuffer *createNewEditorBuffer(char *fileName, char *preLoadedFromFile, time_t modificationTime,
                                           size_t size);
extern EditorBuffer *findOrCreateAndLoadEditorBufferForFile(char *fileName);
extern EditorBuffer *openEditorBufferFromPreload(char *fileName, char *preLoadedFromFile);
extern EditorBuffer *getOpenedAndLoadedEditorBuffer(char *name);
extern void renameEditorBuffer(EditorBuffer *buffer, char *newName, EditorUndo **undo);

extern void setEditorBufferModified(EditorBuffer *buffer);

#endif
