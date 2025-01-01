#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "editor.h"
#include "editorbuffer.h"

#include "editorbuffertable.h"
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
    FileItem dummy_fileitem;
    FileItem *dummy_fileitemP = &dummy_fileitem;
    expect(getFileItemWithFileNumber, will_return(dummy_fileitemP), times(2));

    expect(getEditorBufferForFile, when(fileName, is_equal_to_string(renamed_filename)),
           will_return(NULL)); /* The new does not exist */
    expect(registerEditorBuffer, when(buffer, is_equal_to(original_buffer)));

    EditorUndo *undo = NULL;
    expect(newUndoRename);

    /* In the last createNewEditorBuffer there are two of these, one for real, one for loaded from */
    expect(normalizeFileName_static, when(name, is_equal_to_string(original_filename)),
           will_return(original_filename));
    expect(normalizeFileName_static, when(name, is_equal_to_string(original_filename)),
           will_return(original_filename));
    expect(registerEditorBuffer);
    expect(addFileNameToFileTable, will_return(45));
    expect(allocateNewEditorBufferTextSpace);

    renameEditorBuffer(original_buffer, renamed_filename, &undo);
}
