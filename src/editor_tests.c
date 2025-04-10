#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/internal/mocks_internal.h>

#include "editor.h"

#include "editorbuffer.h"
#include "usage.h"

#include "commons.mock"
#include "cxref.mock"
#include "editorbuffer.mock"
#include "editorbuffertable.mock"
#include "encoding.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "log.h"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "reference.mock"
#include "undo.mock"
#include "yylex.mock"


Describe(Editor);
BeforeEach(Editor) {
    log_set_level(LOG_ERROR);
}
AfterEach(Editor) {}

Ensure(Editor, can_create_new_editor_region_list) {
    EditorBuffer      buffer     = (EditorBuffer){.markers = NULL};
    EditorMarker     *begin      = newEditorMarker(&buffer, 1);
    EditorMarker     *end        = newEditorMarker(&buffer, 10);
    EditorRegionList *regionList = newEditorRegionList(begin, end, NULL);

    end->previous = begin;

    assert_that(regionList->region.begin->buffer, is_equal_to(&buffer));
    assert_that(regionList->region.end->buffer, is_equal_to(&buffer));
}

Ensure(Editor, can_sort_empty_list_of_regions) {
    EditorRegionList *list = NULL;
    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_null);
}

Ensure(Editor, can_sort_single_region) {
    EditorRegionList regionList = {.next = NULL};
    EditorRegionList *list = &regionList;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(&regionList));
}

static EditorRegionList *createEditorRegionList(EditorBuffer *buffer, int begin, int end, EditorRegionList *next) {
    EditorMarker *b = newEditorMarker(buffer, begin);
    EditorMarker *e = newEditorMarker(buffer, end);
    *b = (EditorMarker){ .buffer = buffer, .offset = begin, .previous = NULL, .next = NULL };
    *e = (EditorMarker){ .buffer = buffer, .offset = end, .previous = NULL, .next = NULL };

    EditorRegionList *r = (EditorRegionList *)malloc(sizeof(EditorRegionList));
    *r = (EditorRegionList){ .region = { .begin = b, .end = e}, .next = next};
    return r;
}

Ensure(Editor, can_sort_two_regions) {
    EditorBuffer *buffer = newEditorBuffer("", 1, "", 0, 0);
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 2, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 3, 4, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_three_regions) {
    EditorBuffer *buffer = newEditorBuffer("", 1, "", 0, 0);
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 2, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 3, 4, regionList1);
    EditorRegionList *regionList3 = createEditorRegionList(buffer, 5, 6, regionList2);
    EditorRegionList *list = regionList3;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_enclosing_regions) {
    EditorBuffer *buffer = newEditorBuffer("", 1, "", 0, 0);
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 4, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 2, 3, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_overlapping_regions) {
    EditorBuffer *buffer = newEditorBuffer("", 1, "", 0, 0);
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 3, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 2, 4, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_convert_empty_list_of_references_to_editor_markers) {
    assert_that(convertReferencesToEditorMarkers(NULL), is_null);
}

Ensure(Editor, can_convert_single_reference_to_editor_marker) {
    int SOME_FILE_NUMBER = 42;
    int SOME_LINE_NUMBER = 43;
    int SOME_COLUMN_NUMBER = 44;

    Reference reference = makeReference(
        (Position){
            .file = SOME_FILE_NUMBER,
            .line = SOME_LINE_NUMBER,
            .col = SOME_COLUMN_NUMBER},
        UsageDefined, NULL);

    FileItem fileItem = (FileItem){.name = "name"};

    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(SOME_FILE_NUMBER)),
           will_return(&fileItem));

    // Expect that findEditorBufferForFile() finds one
    char *name = "name";  /* Closable() depends on them being the same pointer */
    EditorBuffer editorBuffer = {.fileName = name, .preLoadedFromFile = NULL};
    expect(findOrCreateAndLoadEditorBufferForFile, when(fileName, is_equal_to_string("name")),
           will_return(&editorBuffer));

    EditorMarkerList *markers = convertReferencesToEditorMarkers(&reference);

    assert_that(markers, is_not_null);
    assert_that(markers->next, is_null);
}

Ensure(Editor, can_allocate_text_space_in_buffer) {
    EditorBuffer *buffer = newEditorBuffer("file", 12, "file", 0, 0);

    allocateNewEditorBufferTextSpace(buffer, 10);

    assert_that(buffer->allocation.bufferSize, is_equal_to(10));
}

Ensure(Editor, can_move_block_in_editor_buffer) {
    char *text = strdup("this is some text");
    int size = strlen(text)+1;
    EditorBuffer *buffer = newEditorBuffer("file", 12, "file", 0, size);
    allocateNewEditorBufferTextSpace(buffer, size);
    strcpy(buffer->allocation.text, text);

    EditorMarker *sourceMarker = newEditorMarker(buffer, 12); /* Space before 'text' */
    EditorMarker *destinationMarker = newEditorMarker(buffer, 7); /* Space before 'some' */

    EditorUndo *undo = NULL;

    always_expect(setEditorBufferModified);
    expect(newUndoMove);

    moveBlockInEditorBuffer(sourceMarker, destinationMarker, 5, &undo);

    assert_that(buffer->allocation.text, is_equal_to_string("this is text some"));
}

Ensure(Editor, can_replace_string_in_editor_buffer) {
    char *text = strdup("this is some text");
    int size = strlen(text)+1;
    EditorBuffer *buffer = newEditorBuffer("file", 12, "file", 0, size);
    allocateNewEditorBufferTextSpace(buffer, size);
    strcpy(buffer->allocation.text, text);

    EditorUndo *undo = NULL;
    expect(newUndoReplace);
    always_expect(setEditorBufferModified);

    replaceStringInEditorBuffer(buffer, 8, 4, "interesting", 11, &undo);

    assert_that(buffer->allocation.text, is_equal_to_string("this is interesting text"));
}

Ensure(Editor, can_replace_string_in_editor_buffer_causing_expansion) {
    char *text = strdup("this is some text");
    int size = strlen(text)+1;
    EditorBuffer *buffer = newEditorBuffer("file", 12, "file", 0, size);
    allocateNewEditorBufferTextSpace(buffer, size);
    strcpy(buffer->allocation.text, text);

    EditorUndo *undo = NULL;
    expect(newUndoReplace);
    always_expect(setEditorBufferModified);

    char replacement[2100];
    for (int i=0; i<2100; i++)
        replacement[i] = 'a';

    replaceStringInEditorBuffer(buffer, 8, 4, replacement, 2100, &undo);

    assert_that(buffer->allocation.text, is_equal_to_string("this is aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa text"));
}
