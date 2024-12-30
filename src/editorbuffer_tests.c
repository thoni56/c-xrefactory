#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "editorbuffer.h"

#include "log.h"

#include "commons.mock"
#include "editor.mock" /* For freeTextSpace */
#include "editorbuffertab.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "undo.mock"


Describe(EditorBuffer);
BeforeEach(EditorBuffer) {
    log_set_level(LOG_ERROR);
}
AfterEach(EditorBuffer) {}

Ensure(EditorBuffer, can_free_empty_list_of_editor_buffers) {
    freeEditorBuffers(NULL);
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

    assert_that(findEditorBufferForFile("existing"), is_not_null);
}
