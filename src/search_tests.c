#include "proto.h"
#include "search.h"

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "session.h"

#include "browsermenu.mock"
#include "commons.mock"
#include "completion.mock"
#include "globals.mock"
#include "match.mock"
#include "options.mock"
#include "reference.mock"
#include "misc.mock"
#include "filetable.mock"
#include "ppc.mock"


Describe(Search);
BeforeEach(Search) {
    log_set_level(LOG_ERROR);
}
AfterEach(Search) {}


Ensure(Search, can_run_empty_test) {
}
