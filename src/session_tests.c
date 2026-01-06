#include "session.h"

/* Unittests */

#include <cgreen/cgreen.h>

#include "log.h"

#include "browsermenu.mock"
#include "commons.mock"
#include "completion.mock"
#include "filetable.mock"
#include "globals.mock"
#include "match.mock"
#include "options.mock"
#include "reference.mock"


Describe(Session);
BeforeEach(Session) {
    log_set_level(LOG_ERROR);
}
AfterEach(Session) {}

Ensure(Session, can_run_an_empty_test) {
}
