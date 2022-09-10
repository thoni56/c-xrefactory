#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "editor.h"

#include "memory.h"

#include "commons.mock"
#include "cxref.mock"
#include "editorbuffertab.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "log.h"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "yylex.mock"

Describe(Editor);
BeforeEach(Editor) {
    log_set_level(LOG_ERROR);
    olcx_memory_init();
}
AfterEach(Editor) {}

Ensure(Editor, can_create_new_editor_region_list) {
    EditorBuffer      buffer;
    EditorMarker     *begin      = newEditorMarker(&buffer, 1, NULL, NULL);
    EditorMarker     *end        = newEditorMarker(&buffer, 10, begin, NULL);
    EditorRegionList *regionList = newEditorRegionList(begin, end, NULL);

    assert_that(regionList->region.begin->buffer, is_equal_to(&buffer));
    assert_that(regionList->region.end->buffer, is_equal_to(&buffer));
}

Ensure(Editor, can_create_buffer_for_non_existing_file) {
    // Given ...
    expect(editorBufferIsMember, will_return(false));

    expect(editorBufferIsMember, will_return(false));

    // editorFindFile()
    expect(fileExists, will_return(true));
    expect(isDirectory, will_return(true));

    // editorCreateNewBuffer()
    expect(normalizeFileName, will_return("non-existant.c"));
    expect(normalizeFileName, will_return("non-existant.c"));
    expect(addEditorBuffer);
    expect(addFileNameToFileTable, when(name, is_equal_to_string("non-existant.c")));

    // When...
    EditorBuffer *buffer = findEditorBufferForFileOrCreate("non-existant.c");

    // Then...
    assert_that(buffer->name, is_equal_to_string("non-existant.c"));
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
    EditorMarker *b = (EditorMarker *)malloc(sizeof(EditorMarker));
    EditorMarker *e = (EditorMarker *)malloc(sizeof(EditorMarker));
    *b = (EditorMarker){ .offset = begin, .previous = NULL, .next = NULL, .buffer = buffer };
    *e = (EditorMarker){ .offset = end, .previous = NULL, .next = NULL, .buffer = buffer };

    EditorRegionList *r = (EditorRegionList *)malloc(sizeof(EditorRegionList));
    *r = (EditorRegionList){ .region = { .begin = b, .end = e}, .next = next};
    return r;
}

Ensure(Editor, can_sort_two_regions) {
    EditorBuffer *buffer = (EditorBuffer *)malloc(sizeof(EditorBuffer));
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 2, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 3, 4, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_three_regions) {
    EditorBuffer *buffer = (EditorBuffer *)malloc(sizeof(EditorBuffer));
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 2, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 3, 4, regionList1);
    EditorRegionList *regionList3 = createEditorRegionList(buffer, 5, 6, regionList2);
    EditorRegionList *list = regionList3;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_enclosing_regions) {
    EditorBuffer *buffer = (EditorBuffer *)malloc(sizeof(EditorBuffer));
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 4, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 2, 3, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}

Ensure(Editor, can_sort_overlapping_regions) {
    EditorBuffer *buffer = (EditorBuffer *)malloc(sizeof(EditorBuffer));
    EditorRegionList *regionList1 = createEditorRegionList(buffer, 1, 3, NULL);
    EditorRegionList *regionList2 = createEditorRegionList(buffer, 2, 4, regionList1);
    EditorRegionList *list = regionList2;

    sortEditorRegionsAndRemoveOverlaps(&list);

    assert_that(list, is_equal_to(regionList1));
}
