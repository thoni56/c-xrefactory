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
    char                      *fileName;    /* In case the content was preloaded this
                                               is the name on disc for the content else
                                               needs to point to the same string as
                                               'realFileName' */
    int                        fileNumber;
    char                      *realFileName;
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
extern void freeEditorBuffer(EditorBuffer *element);
extern EditorBuffer *newEditorBuffer(char *fileName, int fileNumber, char *realFileName, time_t modificationTime,
                                     size_t size);
extern EditorBuffer *createNewEditorBuffer(char *fileName, char *realfileName, time_t modificationTime,
                                           size_t size);
extern EditorBuffer *findEditorBufferForFile(char *name);
extern EditorBuffer *openEditorBufferFromPreload(char *name, char *fileName);
extern EditorBuffer *getEditorBufferFor(char *name);
extern EditorBuffer *getOpenedAndLoadedEditorBuffer(char *name);
extern void renameEditorBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo);

/* Register and deregister will hide the lists that the hashlist have
 * to have while the EditorBuffer still belongs to the caller to
 * create and delete */
extern int registerEditorBuffer(EditorBuffer *buffer);
extern void deregisterEditorBuffer(EditorBuffer *buffer);

// Hopefully temporary
void setEditorBufferModified(EditorBuffer *buffer);

#endif
