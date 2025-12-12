#include <cgreen/cgreen.h>

#include "sessionstack.h"

#include "log.h"

#include "browsermenu.mock"
#include "completion.mock"
#include "globals.mock"
#include "options.mock"
#include "reference.mock"


Describe(SessionStack);
BeforeEach(SessionStack) {
    log_set_level(LOG_ERROR);
}
AfterEach(SessionStack) {}

Ensure(SessionStack, can_run_an_empty_test) {
}
