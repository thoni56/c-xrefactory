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
    char                      *loadedFromFile;   /* In case the content was preloaded
                                               this is the name on disc for the content
                                               else needs to point to the same string as
                                               'realFileName'. TODO should be NULL in
                                               that case. */
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


extern void fillEmptyEditorBuffer(EditorBuffer *buffer, char *fileName, int fileNumber, char *realFileName);
extern void freeEditorBuffer(EditorBuffer *buffer);
extern EditorBuffer *newEditorBuffer(char *fileName, int fileNumber, char *realFileName, time_t modificationTime,
                                     size_t size);
extern EditorBuffer *createNewEditorBuffer(char *fileName, char *realfileName, time_t modificationTime,
                                           size_t size);
extern EditorBuffer *findOrCreateAndLoadEditorBufferForFile(char *name);
extern EditorBuffer *openEditorBufferFromPreload(char *name, char *fileName);
extern EditorBuffer *getOpenedAndLoadedEditorBuffer(char *name);
extern void renameEditorBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo);

// Hopefully temporary
void setEditorBufferModified(EditorBuffer *buffer);

#endif
