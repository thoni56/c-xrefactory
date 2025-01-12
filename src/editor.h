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



// EditorBuffer functions...
extern void replaceStringInEditorBuffer(EditorBuffer *buff, int position, int delsize, char *str,
                                        int strlength, EditorUndo **undo);
extern void moveBlockInEditorBuffer(EditorMarker *src, EditorMarker *dest, int size, EditorUndo **undo);
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

extern EditorMarkerList *convertReferencesToEditorMarkers(Reference *references);
extern Reference        *convertEditorMarkersToReferences(EditorMarkerList **markerList);

extern void          editorDumpRegionList(EditorRegionList *mml);

extern void removeBlanksAtEditorMarker(EditorMarker *mm, int direction, EditorUndo **undo);
extern void          editorDumpUndoList(EditorUndo *uu);

extern bool editorMapOnNonExistantFiles(char *dirname, void (*fun)(MAP_FUN_SIGNATURE), SearchDepth depth, char *a1,
                                        char *a2, Completions *a3, void *a4, int *a5);

#endif
