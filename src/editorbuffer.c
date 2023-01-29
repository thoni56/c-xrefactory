#include "editorbuffer.h"

#include "editor.h"
#include "log.h"
#include "memory.h"


static void checkForMagicMarker(EditorBufferAllocationData *allocation) {
    assert(allocation->allocatedBlock[allocation->allocatedSize] == 0x3b);
}


void freeEditorBuffer(EditorBufferList *list) {
    if (list == NULL)
        return;
    log_trace("freeing buffer %s==%s", list->buffer->name, list->buffer->fileName);
    if (list->buffer->fileName != list->buffer->name) {
        editorFree(list->buffer->fileName, strlen(list->buffer->fileName)+1);
    }
    editorFree(list->buffer->name, strlen(list->buffer->name)+1);
    for (EditorMarker *marker=list->buffer->markers; marker!=NULL;) {
        EditorMarker *next = marker->next;
        editorFree(marker, sizeof(EditorMarker));
        marker = next;
    }
    if (list->buffer->textLoaded) {
        log_trace("freeing %d of size %d", list->buffer->allocation.allocatedBlock,
                  list->buffer->allocation.allocatedSize);
        checkForMagicMarker(&list->buffer->allocation);
        freeTextSpace(list->buffer->allocation.allocatedBlock,
                      list->buffer->allocation.allocatedIndex);
    }
    editorFree(list->buffer, sizeof(EditorBuffer));
    editorFree(list, sizeof(EditorBufferList));
}
