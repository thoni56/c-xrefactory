#ifndef EDITOR_H
#define EDITOR_H

#include "proto.h"

extern void editorInit();
extern int statb(char *path, struct stat  *statbuf);
extern int editorMarkerLess(S_editorMarker *m1, S_editorMarker *m2);
extern int editorMarkerLessOrEq(S_editorMarker *m1, S_editorMarker *m2);
extern int editorMarkerGreater(S_editorMarker *m1, S_editorMarker *m2);
extern int editorMarkerGreaterOrEq(S_editorMarker *m1, S_editorMarker *m2);
extern int editorMarkerListLess(S_editorMarkerList *l1, S_editorMarkerList *l2);
extern int editorRegionListLess(S_editorRegionList *l1, S_editorRegionList *l2);
extern S_editorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName);
extern S_editorBuffer *editorGetOpenedBuffer(char *name);
extern S_editorBuffer *editorGetOpenedAndLoadedBuffer(char *name);
extern S_editorBuffer *editorFindFile(char *name);
extern S_editorBuffer *editorFindFileCreate(char *name);
extern S_editorMarker *editorCrNewMarkerForPosition(S_position *pos);
extern S_editorMarkerList *editorReferencesToMarkers(S_reference *refs, int (*filter)(S_reference *, void *), void *filterParam);
extern S_reference *editorMarkersToReferences(S_editorMarkerList **mms);
extern void editorRenameBuffer(S_editorBuffer *buff, char *newName, S_editorUndo **undo);
extern void editorReplaceString(S_editorBuffer *buff, int position, int delsize, char *str, int strlength, S_editorUndo **undo);
extern void editorMoveBlock(S_editorMarker *dest, S_editorMarker *src, int size, S_editorUndo **undo);
extern void editorDumpBuffer(S_editorBuffer *buff);
extern void editorDumpBuffers();
extern void editorDumpMarker(S_editorMarker *mm);
extern void editorDumpMarkerList(S_editorMarkerList *mml);
extern void editorDumpRegionList(S_editorRegionList *mml);
extern void editorQuasiSaveModifiedBuffers();
extern void editorLoadAllOpenedBufferFiles();
extern S_editorMarker *editorCrNewMarker(S_editorBuffer *buff, int offset);
extern S_editorMarker *editorDuplicateMarker(S_editorMarker *mm);
extern int editorCountLinesBetweenMarkers(S_editorMarker *m1, S_editorMarker *m2);
extern int editorRunWithMarkerUntil(S_editorMarker *m, int (*until)(int), int step);
extern int editorMoveMarkerToNewline(S_editorMarker *m, int direction);
extern int editorMoveMarkerToNonBlank(S_editorMarker *m, int direction);
extern int editorMoveMarkerBeyondIdentifier(S_editorMarker *m, int direction);
extern int editorMoveMarkerToNonBlankOrNewline(S_editorMarker *m, int direction);
extern void editorRemoveBlanks(S_editorMarker *mm, int direction, S_editorUndo **undo);
extern void editorDumpUndoList(S_editorUndo *uu);
extern void editorMoveMarkerToLineCol(S_editorMarker *m, int line, int col);
extern void editorMarkersDifferences(
                                     S_editorMarkerList **list1, S_editorMarkerList **list2,
                                     S_editorMarkerList **diff1, S_editorMarkerList **diff2);
extern void editorFreeMarker(S_editorMarker *marker);
extern void editorFreeMarkerListNotMarkers(S_editorMarkerList *occs);
extern void editorFreeMarkersAndRegionList(S_editorRegionList *occs);
extern void editorFreeRegionListNotMarkers(S_editorRegionList *occs);
extern void editorSortRegionsAndRemoveOverlaps(S_editorRegionList **regions);
extern void editorSplitMarkersWithRespectToRegions(
                                                   S_editorMarkerList  **inMarkers,
                                                   S_editorRegionList  **inRegions,
                                                   S_editorMarkerList  **outInsiders,
                                                   S_editorMarkerList  **outOutsiders
                                                   );
extern void editorRestrictMarkersToRegions(S_editorMarkerList **mm, S_editorRegionList **regions);
extern S_editorMarker *editorCrMarkerForBufferBegin(S_editorBuffer *buffer);
extern S_editorMarker *editorCrMarkerForBufferEnd(S_editorBuffer *buffer);
extern S_editorRegionList *editorWholeBufferRegion(S_editorBuffer *buffer);
extern void editorScheduleModifiedBuffersToUpdate();
extern void editorFreeMarkersAndMarkerList(S_editorMarkerList *occs);
extern int editorMapOnNonexistantFiles(
                                       char *dirname,
                                       void (*fun)(MAP_FUN_PROFILE),
                                       int deep,
                                       char *a1,
                                       char *a2,
                                       S_completions *a3,
                                       void *a4,
                                       int *a5
                                       );
extern void editorCloseBufferIfClosable(char *name);
extern void editorCloseAllBuffersIfClosable();
extern void editorCloseAllBuffers();

#endif
