#include "editormarker.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "commons.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "ppc.h"



void attachMarkerToBuffer(EditorMarker *marker, EditorBuffer *buffer) {
    marker->buffer = buffer;
    marker->next = buffer->markers;
    buffer->markers = marker;
    marker->previous = NULL;
    if (marker->next!=NULL)
        marker->next->previous = marker;
}

EditorMarker *newEditorMarker(EditorBuffer *buffer, unsigned offset) {
    EditorMarker *marker;

    marker = malloc(sizeof(EditorMarker));
    marker->buffer = buffer;
    marker->offset = offset;
    marker->previous = NULL;
    marker->next = NULL;

    attachMarkerToBuffer(marker, buffer);

    return marker;
}

EditorMarker *newEditorMarkerForPosition(Position position) {
    EditorBuffer *buffer;
    EditorMarker *marker;

    if (position.file==NO_FILE_NUMBER || position.file<0) {
        errorMessage(ERR_INTERNAL, "[editor] creating marker for non-existent position");
    }
    buffer = findOrCreateAndLoadEditorBufferForFile(getFileItemWithFileNumber(position.file)->name);
    marker = newEditorMarker(buffer, 0);
    moveEditorMarkerToLineAndColumn(marker, position.line, position.col);
    return marker;
}

EditorMarker *createEditorMarkerForBufferBegin(EditorBuffer *buffer) {
    return newEditorMarker(buffer, 0);
}

EditorMarker *createEditorMarkerForBufferEnd(EditorBuffer *buffer) {
    return newEditorMarker(buffer, buffer->allocation.bufferSize);
}

EditorRegionList *createEditorRegionForWholeBuffer(EditorBuffer *buffer) {
    EditorMarker *bufferBegin, *bufferEnd;
    EditorRegion theBufferRegion;
    EditorRegionList *theBufferRegionList;

    bufferBegin     = createEditorMarkerForBufferBegin(buffer);
    bufferEnd       = createEditorMarkerForBufferEnd(buffer);
    theBufferRegion = (EditorRegion){.begin = bufferBegin, .end = bufferEnd};
    theBufferRegionList = malloc(sizeof(EditorRegionList));
    *theBufferRegionList = (EditorRegionList){.region = theBufferRegion, .next = NULL};

    return theBufferRegionList;
}

EditorMarker *duplicateEditorMarker(EditorMarker *marker) {
    return newEditorMarker(marker->buffer, marker->offset);
}

EditorRegionList *newEditorRegionList(EditorMarker *begin, EditorMarker *end, EditorRegionList *next) {
    EditorRegionList *regionList;

    regionList = malloc(sizeof(EditorRegionList));
    regionList->region.begin = begin;
    regionList->region.end = end;
    regionList->next = next;

    return regionList;
}

void moveEditorMarkerToLineAndColumn(EditorMarker *marker, int line, int col) {
    char *text, *textMax;
    int ln;
    EditorBuffer *buffer;
    int c;

    assert(marker);
    buffer = marker->buffer;
    text      = buffer->allocation.text;
    textMax   = text + buffer->allocation.bufferSize;
    ln = 1;
    if (line > 1) {
        for (; text < textMax; text++) {
            if (*text == '\n') {
                ln++;
                if (ln == line)
                    break;
            }
        }
        if (text < textMax)
            text++;
    }
    c = 0;
    for (; text < textMax && c < col; text++, c++) {
        if (*text == '\n')
            break;
    }
    marker->offset = text - buffer->allocation.text;
    assert(marker->offset >= 0 && marker->offset <= buffer->allocation.bufferSize);
}

bool editorMarkerBefore(EditorMarker *m1, EditorMarker *m2) {
    // following is tricky as it works also for renamed buffers
    if (m1->buffer < m2->buffer)
        return true;
    if (m1->buffer > m2->buffer)
        return false;
    if (m1->offset < m2->offset)
        return true;
    if (m1->offset > m2->offset)
        return false;
    return false;
}

bool editorMarkerAfter(EditorMarker *m1, EditorMarker *m2) {
    return editorMarkerBefore(m2, m1);
}

bool editorMarkerListBefore(EditorMarkerList *l1, EditorMarkerList *l2) {
    return editorMarkerBefore(l1->marker, l2->marker);
}

static bool runWithEditorMarkerUntil(EditorMarker *marker, int (*until)(int), int step) {
    int   offset, max;
    char *text;

    assert(step == -1 || step == 1);
    offset = marker->offset;
    max    = marker->buffer->allocation.bufferSize;
    text   = marker->buffer->allocation.text;
    while (offset >= 0 && offset < max && (*until)(text[offset]) == 0)
        offset += step;
    marker->offset = offset;
    if (offset < 0) {
        marker->offset = 0;
        return false;
    }
    if (offset >= max) {
        marker->offset = max - 1;
        return false;
    }
    return true;
}

static int isNewLine(int c) { return c == '\n'; }
int moveEditorMarkerToNewline(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNewLine, direction);
}

static int isNonBlank(int c) {return ! isspace(c);}
int moveEditorMarkerToNonBlank(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNonBlank, direction);
}

static int isNonBlankOrNewline(int c) {return c=='\n' || ! isspace(c);}
int        moveEditorMarkerToNonBlankOrNewline(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNonBlankOrNewline, direction);
}

static int isNotIdentPart(int c) {return !isalnum(c) && c!='_' && c!='$';}
int        moveEditorMarkerBeyondIdentifier(EditorMarker *m, int direction) {
    return runWithEditorMarkerUntil(m, isNotIdentPart, direction);
}

int countLinesBetweenEditorMarkers(EditorMarker *m1, EditorMarker *m2) {
    // this can happen after an error in moving, just pass in this case
    if (m1 == NULL || m2 == NULL)
        return 0;
    assert(m1->buffer == m2->buffer);
    assert(m1->offset <= m2->offset);
    char *text  = m1->buffer->allocation.text;
    int   max   = m2->offset;
    int   count = 0;
    for (int i = m1->offset; i < max; i++) {
        if (text[i] == '\n')
            count++;
    }
    return count;
}

static bool editorRegionListBefore(EditorRegionList *l1, EditorRegionList *l2) {
    if (editorMarkerBefore(l1->region.begin, l2->region.begin))
        return true;
    if (editorMarkerBefore(l2->region.begin, l1->region.begin))
        return false;
    // region beginnings are equal, check end
    if (editorMarkerBefore(l1->region.end, l2->region.end))
        return true;
    if (editorMarkerBefore(l2->region.end, l1->region.end))
        return false;
    return false;
}

void sortEditorRegionsAndRemoveOverlaps(EditorRegionList **regions) {
    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListBefore);
    for (EditorRegionList *region = *regions; region != NULL; region = region->next) {
        EditorRegionList *next = region->next;
        if (next != NULL && region->region.begin->buffer == next->region.begin->buffer) {
            assert(region->region.begin->buffer
                   == region->region.end->buffer); // region consistency check
            assert(next->region.begin->buffer
                   == next->region.end->buffer); // region consistency check
            assert(region->region.begin->offset <= next->region.begin->offset);
            EditorMarker *newEnd = NULL;
            if (next->region.end->offset <= region->region.end->offset) {
                // second inside first
                newEnd = region->region.end;
                freeEditorMarker(next->region.begin);
                freeEditorMarker(next->region.end);
            } else if (next->region.begin->offset <= region->region.end->offset) {
                // they have common part
                newEnd = next->region.end;
                freeEditorMarker(next->region.begin);
                freeEditorMarker(region->region.end);
            }
            if (newEnd != NULL) {
                region->region.end = newEnd;
                region->next       = next->next;
                free(next);
                next = NULL;
                continue;
            }
        }
    }
}

#if 0
static void editorDumpRegionList(EditorRegionList *mml) {
    log_trace("[dumping editor regions]");
    for (EditorRegionList *mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->region.begin == NULL || mm->region.end == NULL) {
            log_trace("%ld: [null]", (unsigned long)mm);
        } else {
            log_trace("%ld: [%s: %d - %d] --> %c - %c", (unsigned long)mm,
                      simpleFileName(mm->region.begin->buffer->fileName), mm->region.begin->offset,
                      mm->region.end->offset, CHAR_ON_MARKER(mm->region.begin), CHAR_ON_MARKER(mm->region.end));
        }
    }
    log_trace("[dumpend] of editor regions]");
}
#endif

static void splitEditorMarkersWithRespectToRegions(EditorMarkerList **inMarkers,
                                                   EditorRegionList **inRegions,
                                                   EditorMarkerList **outInsiders,
                                                   EditorMarkerList **outOutsiders) {
    EditorMarkerList *markers1, *markers2;
    EditorRegionList *regions;

    *outInsiders = NULL;
    *outOutsiders = NULL;

    LIST_MERGE_SORT(EditorMarkerList, *inMarkers, editorMarkerListBefore);
    sortEditorRegionsAndRemoveOverlaps(inRegions);

    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *inMarkers);

    //& editorDumpRegionList(*inRegions);
    //& editorDumpMarkerList(*inMarkers);

    regions = *inRegions;
    markers1= *inMarkers;
    while (markers1!=NULL) {
        markers2 = markers1->next;
        while (regions!=NULL && editorMarkerAfter(regions->region.begin, markers1->marker))
            regions = regions->next;
        if (regions!=NULL && editorMarkerAfter(regions->region.end, markers1->marker)) {
            // is inside
            markers1->next = *outInsiders;
            *outInsiders = markers1;
        } else {
            // is outside
            markers1->next = *outOutsiders;
            *outOutsiders = markers1;
        }
        markers1 = markers2;
    }

    *inMarkers = NULL;
    LIST_REVERSE(EditorRegionList, *inRegions);
    LIST_REVERSE(EditorMarkerList, *outInsiders);
    LIST_REVERSE(EditorMarkerList, *outOutsiders);
    //&editorDumpMarkerList(*outInsiders);
    //&editorDumpMarkerList(*outOutsiders);
}

void restrictEditorMarkersToRegions(EditorMarkerList **mm, EditorRegionList **regions) {
    EditorMarkerList *ins, *outs;
    splitEditorMarkersWithRespectToRegions(mm, regions, &ins, &outs);
    *mm = ins;
    freeEditorMarkerListAndMarkers(outs);
}

static EditorMarkerList *combineEditorMarkerLists(EditorMarkerList **diff, EditorMarkerList *list) {
    EditorMarker     *marker = newEditorMarker(list->marker->buffer, list->marker->offset);
    EditorMarkerList *l;

    l = malloc(sizeof(EditorMarkerList));
    *l    = (EditorMarkerList){.marker = marker, .usage = list->usage, .next = *diff};
    *diff = l;
    list  = list->next;

    return list;
}

void editorMarkersDifferences(EditorMarkerList **list1, EditorMarkerList **list2,
                              EditorMarkerList **diff1, EditorMarkerList **diff2) {
    EditorMarkerList *l1, *l2;

    LIST_MERGE_SORT(EditorMarkerList, *list1, editorMarkerListBefore);
    LIST_MERGE_SORT(EditorMarkerList, *list2, editorMarkerListBefore);
    *diff1 = *diff2 = NULL;
    for(l1 = *list1, l2 = *list2; l1!=NULL && l2!=NULL; ) {
        if (editorMarkerListBefore(l1, l2)) {
            EditorMarker *marker = newEditorMarker(l1->marker->buffer, l1->marker->offset);
            EditorMarkerList *l;
            l = malloc(sizeof(EditorMarkerList)); /* TODO1 */
            *l = (EditorMarkerList){.marker = marker, .usage = l1->usage, .next = *diff1};
            *diff1 = l;
            l1 = l1->next;
        } else if (editorMarkerListBefore(l2, l1)) {
            l2 = combineEditorMarkerLists(diff2, l2);
        } else {
            l1 = l1->next;
            l2 = l2->next;
        }
    }
    while (l1 != NULL) {
        l1 = combineEditorMarkerLists(diff1, l1);
    }
    while (l2 != NULL) {
        EditorMarker *marker = newEditorMarker(l2->marker->buffer, l2->marker->offset);
        EditorMarkerList *l;
        l = malloc(sizeof(EditorMarkerList));
        *l = (EditorMarkerList){.marker = marker, .usage = l2->usage, .next = *diff2};
        *diff2 = l;
        l2 = l2->next;
    }
}

void removeEditorMarkerFromBufferWithoutFreeing(EditorMarker *marker) {
    if (marker == NULL)
        return;
    if (marker->next != NULL)
        marker->next->previous = marker->previous;
    if (marker->previous == NULL) {
        marker->buffer->markers = marker->next;
    } else {
        marker->previous->next = marker->next;
    }
}

void freeEditorMarker(EditorMarker *marker) {
    if (marker == NULL)
        return;
    removeEditorMarkerFromBufferWithoutFreeing(marker);
    free(marker);
}


void freeEditorMarkerListButNotMarkers(EditorMarkerList *occs) {
    for (EditorMarkerList *o = occs; o != NULL;) {
        EditorMarkerList *next = o->next; /* Save next as we are freeing 'o' */
        free(o);
        o = next;
    }
}

void freeEditorMarkerListAndMarkers(EditorMarkerList *occs) {
    for (EditorMarkerList *o = occs; o != NULL;) {
        EditorMarkerList *next = o->next; /* Save next as we are freeing 'o' */
        freeEditorMarker(o->marker);
        free(o);
        o = next;
    }
}

void freeEditorMarkersAndRegionList(EditorRegionList *occs) {
    for (EditorRegionList *o = occs; o != NULL;) {
        EditorRegionList *next = o->next; /* Save next as we are freeing 'o' */
        freeEditorMarker(o->region.begin);
        freeEditorMarker(o->region.end);
        free(o);
        o = next;
    }
}

void editorDumpMarker(EditorMarker *mm) {
    char tmpBuff[TMP_BUFF_SIZE];

    sprintf(tmpBuff, "[%s:%d] --> %c", simpleFileName(mm->buffer->fileName), mm->offset, CHAR_ON_MARKER(mm));
    ppcBottomInformation(tmpBuff);
}

void editorDumpMarkerList(EditorMarkerList *mml) {
    log_trace("[dumping editor markers]");
    for (EditorMarkerList *mm=mml; mm!=NULL; mm=mm->next) {
        if (mm->marker == NULL) {
            log_trace("[null]");
        } else {
            log_trace("[%s:%d] --> %c", simpleFileName(mm->marker->buffer->fileName), mm->marker->offset,
                      CHAR_ON_MARKER(mm->marker));
        }
    }
    log_trace("[dumpend of editor marker]");
}
