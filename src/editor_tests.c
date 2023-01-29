#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "access.h"
#include "editor.h"

#include "memory.h"

#include "commons.mock"
#include "cxref.mock"
#include "editorbuffer.mock"
#include "editorbuffertab.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "log.h"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "reference.h"
#include "reference.mock"
#include "usage.h"
#include "yylex.mock"


Describe(Editor);
BeforeEach(Editor) {
    log_set_level(LOG_ERROR);
    olcxMemoryInit();
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

Ensure(Editor, can_convert_empty_list_of_references_to_editor_markers) {
    assert_that(convertReferencesToEditorMarkers(NULL), is_null);
}

Ensure(Editor, can_convert_single_reference_to_editor_marker) {
    Usage usage;
    fillUsage(&usage, UsageDefined, AccessDefault);
    Reference reference;
    int SOME_FILE_NUMBER = 42;
    int SOME_LINE_NUMBER = 43;
    int SOME_COLUMN_NUMBER = 44;
    fillReference(&reference, usage, (Position){.file = SOME_FILE_NUMBER, .line = SOME_LINE_NUMBER,
            .col = SOME_COLUMN_NUMBER}, NULL);
    FileItem fileItem = (FileItem){.name = "name"};

    expect(getFileItem, when(fileNumber, is_equal_to(SOME_FILE_NUMBER)),
           will_return(&fileItem));

    // Expect that there is no editorbuffer for "fileNumber"
    always_expect(editorBufferIsMember, when(editorBuffer_name, is_equal_to_string("name")),
                  will_return(NULL));

    // But there is a plain file
    expect(fileExists, when(fullPath, is_equal_to_string("name")),
           will_return(true));
    expect(isDirectory, when(fullPath, is_equal_to_string("name")),
           will_return(false));

    // Now create an EditorBuffer for it
    int SOME_FILE_MODIFICATION_TIME = 2222;
    int SOME_FILE_SIZE = 3333;
    expect(fileModificationTime, when(path, is_equal_to_string("name")),
           will_return(SOME_FILE_MODIFICATION_TIME));
    expect(fileSize, when(path, is_equal_to_string("name")),
           will_return(SOME_FILE_SIZE));

    // createNewEditorBuffer() will compare name and fileName
    expect(normalizeFileName, when(name, is_equal_to_string("name")),
           will_return("name"));
    expect(normalizeFileName, when(name, is_equal_to_string("name")),
           will_return("name"));

    int SOME_EDITOR_BUFFER_INDEX = 1;
    expect(addEditorBuffer, will_return(SOME_EDITOR_BUFFER_INDEX));
    int SOME_FILE_TABLE_INDEX = 13;
    expect(addFileNameToFileTable, will_return(SOME_FILE_TABLE_INDEX));

    expect(isDirectory, when(fullPath, is_equal_to_string("name")),
           will_return(false));
    expect(fileSize, when(path, is_equal_to_string("name")),
           will_return(SOME_FILE_SIZE));
    expect(fileModificationTime, when(path, is_equal_to_string("name")),
           will_return(SOME_FILE_MODIFICATION_TIME));
    expect(fileSize, when(path, is_equal_to_string("name")),
           will_return(SOME_FILE_SIZE));

    FILE *file;
    expect(openFile, when(fileName, is_equal_to_string("name")),
           will_return(&file));
    expect(readFile, when(file, is_equal_to(&file)), when(size, is_equal_to(1)), when(count, is_equal_to(SOME_FILE_SIZE)),
           will_return(SOME_FILE_SIZE));
    expect(readFile, when(file, is_equal_to(&file)), when(size, is_equal_to(1)), when(count, is_equal_to(0)),
           will_return(0));
    expect(closeFile);

    EditorMarkerList *markers = convertReferencesToEditorMarkers(&reference);

    assert_that(markers, is_not_null);
    assert_that(markers->next, is_null);
}
