#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include <sys/stat.h>

#include "head.h"
#include "argumentsvector.h"
#include "completion.h"
#include "contenttoken.h"
#include "editorbuffer.h"
#include "editormarker.h"
#include "proto.h"
#include "rebuild.h"
#include "reference.h"
#include "undo.h"


extern void editorInit(void);

extern bool   editorFileExists(char *path);
extern size_t editorFileSize(char *path);
extern FileTimestamp editorFileModificationTime(char *path);
extern ContentToken editorContentTokenOf(char *path);

extern void replaceStringInEditorBuffer(EditorBuffer *buff, int position, int delsize, char *str,
                                        int strlength, EditorUndo **undo);
extern void moveBlockInEditorBuffer(EditorMarker *src, EditorMarker *dest, int size, EditorUndo **undo);
extern void loadAllOpenedEditorBuffers(void);
extern void closeAllEditorBuffersIfClosable(void);
extern void clearHeldThisRequestFlags(void);
extern void closeEditorBuffersNoLongerHeld(BuildList *list);

extern void freeTextSpace(char *space, int index);
extern void loadFileIntoEditorBuffer(EditorBuffer *buffer, FileTimestamp modificationTime, size_t fileSize);
extern void loadTextIntoEditorBuffer(EditorBuffer *buffer, FileTimestamp modificationTime, const char *text);
extern void allocateNewEditorBufferTextSpace(EditorBuffer *buffer, int size);

extern EditorMarkerList *convertReferencesToEditorMarkers(Reference *references);
extern Reference *convertEditorMarkersToReferences(EditorMarkerList **markerList);

extern void editorDumpUndoList(EditorUndo *uu);

#endif
