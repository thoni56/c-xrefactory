#include "browsingmenu.h"

/* Unittests */

#include <cgreen/cgreen.h>

#include "log.h"

#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "reference.mock"


Describe(BrowsingMenu);
BeforeEach(BrowsingMenu) {
    log_set_level(LOG_ERROR);
}
AfterEach(BrowsingMenu) {}

Ensure(BrowsingMenu, can_run_empty_test) {
}
