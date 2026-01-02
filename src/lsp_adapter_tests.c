#include "lsp_adapter.h"

/* Unittests for lsp_adapter */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>
#include <cjson/cJSON.h>

#include "filetable.mock"
#include "reference_database.mock"
#include "commons.mock"


Describe(LspAdapter);
BeforeEach(LspAdapter) {}
AfterEach(LspAdapter) {}


Ensure(LspAdapter, findDefinition_returns_a_location) {
    fail_test("Not implemented yet");
}
