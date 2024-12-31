#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "editorbuffer.h"

#include "log.h"

#include "commons.mock"
#include "editor.mock" /* For freeTextSpace */
#include "editorbuffertable.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "undo.mock"


Describe(EditorBuffer);
BeforeEach(EditorBuffer) {
    log_set_level(LOG_ERROR);
}
AfterEach(EditorBuffer) {}

Ensure(EditorBuffer, can_free_null_editor_buffer) {
    freeEditorBuffer(NULL);
}

Ensure(EditorBuffer, will_return_null_for_non_existing_file) {
    always_expect(editorBufferIsMember, will_return(false));
    expect(fileExists, will_return(false));

    assert_that(findEditorBufferForFile("non-existing"), is_null);
}

Ensure(EditorBuffer, will_create_buffer_and_load_existing_file) {
    always_expect(editorBufferIsMember, will_return(false));
    expect(fileExists, will_return(true));
    always_expect(isDirectory, will_return(false));
    always_expect(fileSize, will_return(42));
    always_expect(fileModificationTime, will_return(666));
    always_expect(normalizeFileName_static, will_return("existing"));
    expect(addEditorBuffer);
    expect(addFileNameToFileTable);
    expect(allocateNewEditorBufferTextSpace);
    expect(loadFileIntoEditorBuffer);

    EditorBuffer *editorBuffer = findEditorBufferForFile("existing");

    assert_that(editorBuffer, is_not_null);
    assert_that(editorBuffer->fileName, is_equal_to_string("existing"));
    assert_that(editorBuffer->size, is_equal_to(42));
}

Ensure(EditorBuffer, can_register_and_deregister_a_buffer) {
    EditorBuffer *registered_buffer = newEditorBuffer("some file", 112, "somefile", 0, 42);
    int some_index = 13;

    EditorBufferList *captured_list_element;
    expect(addEditorBuffer, will_return(some_index), will_capture_parameter(bufferList, captured_list_element));

    int registered_index = registerEditorBuffer(registered_buffer);

    assert_that(registered_index, is_equal_to(some_index));
    assert_that(captured_list_element->buffer, is_equal_to(registered_buffer));
}
