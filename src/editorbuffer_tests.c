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
