#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "editorbuffer.h"

#include "log.h"

#include "editor.mock"          /* For freeTextSpace */


Describe(EditorBuffer);
BeforeEach(EditorBuffer) {
    log_set_level(LOG_ERROR);
}
AfterEach(EditorBuffer) {}

Ensure(EditorBuffer, can_run_an_empty_test) {
}
