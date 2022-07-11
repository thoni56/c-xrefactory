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
    EditorMarker *    begin      = newEditorMarker(&buffer, 1, NULL, NULL);
    EditorMarker *    end        = newEditorMarker(&buffer, 10, begin, NULL);
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
    EditorBuffer *buffer = editorFindFileCreate("non-existant.c");

    // Then...
    assert_that(buffer->name, is_equal_to_string("non-existant.c"));
}
