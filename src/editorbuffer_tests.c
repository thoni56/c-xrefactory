#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "editorbuffer.h"
#include "log.h"

#include "commons.mock"
#include "editor.mock"
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

Ensure(EditorBuffer, can_rename_a_buffer_to_non_existing_buffer) {
    cwd[0] = '\0';
    char *renamed_filename = "renamed.c";
    expect(normalizeFileName_static, when(name, is_equal_to_string(renamed_filename)),
           will_return(renamed_filename));

    char *original_filename = "original.c";
    EditorBuffer *original_buffer = malloc(sizeof(EditorBuffer));
    *original_buffer = (EditorBuffer){.fileName = original_filename};

    expect(getEditorBufferForFile, when(fileName, is_equal_to_string(original_filename)),
           will_return(original_buffer));

    expect(deregisterEditorBuffer, when(fileName, is_equal_to_string(original_filename)),
           will_return(original_buffer));

    expect(addFileNameToFileTable, will_return(45));
    expect(markFileAsDeleted);

    expect(getEditorBufferForFile, when(fileName, is_equal_to_string(renamed_filename)),
           will_return(NULL)); /* The new does not exist */
    expect(registerEditorBuffer, when(buffer, is_equal_to(original_buffer)));

    EditorUndo *undo = NULL;
    expect(newUndoRename);

    /* In the last createNewEditorBuffer there are two of these, one for real, one for loaded from */
    expect(normalizeFileName_static, when(name, is_equal_to_string(original_filename)),
           will_return(original_filename));
    expect(registerEditorBuffer);
    expect(addFileNameToFileTable, will_return(45));
    expect(allocateNewEditorBufferTextSpace);

    renameEditorBuffer(original_buffer, renamed_filename, &undo);
}

/* A buffer can be registered before its text is read, e.g. by -preload, so the
 * lookup that promises a loaded buffer loads it */
Ensure(EditorBuffer, loads_a_registered_buffer_without_text_from_its_preload) {
    EditorBuffer buffer = {.fileName = "held.c", .preLoadedFromFile = "held.preload", .textLoaded = false};
    FileTimestamp preloadTime = {.tv_sec = 1000, .tv_nsec = 0};
    expect(getEditorBufferForFile, when(fileName, is_equal_to_string("held.c")), will_return(&buffer));
    expect(fileExists, when(fullPath, is_equal_to_string("held.preload")), will_return(true));
    expect(fileModificationTime, when(path, is_equal_to_string("held.preload")), will_return(&preloadTime));
    expect(fileSize, when(path, is_equal_to_string("held.preload")), will_return(42));
    expect(loadFileIntoEditorBuffer, when(buffer, is_equal_to(&buffer)), when(fileSize, is_equal_to(42)));

    assert_that(findOrCreateAndLoadEditorBufferForFile("held.c"), is_equal_to(&buffer));
}

Ensure(EditorBuffer, loads_a_registered_buffer_without_text_and_without_preload_from_disk) {
    EditorBuffer buffer = {.fileName = "source.c", .textLoaded = false};
    FileTimestamp diskTime = {.tv_sec = 2000, .tv_nsec = 0};
    expect(getEditorBufferForFile, when(fileName, is_equal_to_string("source.c")), will_return(&buffer));
    expect(fileExists, when(fullPath, is_equal_to_string("source.c")), will_return(true));
    expect(fileModificationTime, when(path, is_equal_to_string("source.c")), will_return(&diskTime));
    expect(fileSize, when(path, is_equal_to_string("source.c")), will_return(17));
    expect(loadFileIntoEditorBuffer, when(buffer, is_equal_to(&buffer)), when(fileSize, is_equal_to(17)));

    assert_that(findOrCreateAndLoadEditorBufferForFile("source.c"), is_equal_to(&buffer));
}
