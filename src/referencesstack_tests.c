#include <cgreen/cgreen.h>

#include "referencesstack.h"

#include "log.h"

#include "completion.mock"
#include "globals.mock"
#include "menu.mock"
#include "options.mock"
#include "reference.mock"


Describe(ReferencesStack);
BeforeEach(ReferencesStack) {
    log_set_level(LOG_ERROR);
}
AfterEach(ReferencesStack) {}

Ensure(ReferencesStack, can_run_an_empty_test) {
}
