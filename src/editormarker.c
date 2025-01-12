#include "editormarker.h"

#include <stdlib.h>
#include <stdio.h>

#include "commons.h"
#include "filetable.h"
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

void moveEditorMarkerToLineAndColumn(EditorMarker *marker, int line, int col) {
    char           *text, *textMax;
    int    ln;
    EditorBuffer   *buffer;
    int             c;

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
