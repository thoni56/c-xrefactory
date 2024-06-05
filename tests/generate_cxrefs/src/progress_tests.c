#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "progress.h"

#include "log.h"

Describe(Progress);
BeforeEach(Progress) {
    log_set_level(LOG_ERROR);
}
AfterEach(Progress) {}

Ensure(Progress, can_run_empty_test) {
}
