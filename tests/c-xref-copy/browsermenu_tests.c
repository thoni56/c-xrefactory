#include <cgreen/cgreen.h>

#include "browsermenu.h"

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


Describe(BrowserMenu);
BeforeEach(BrowserMenu) {
    log_set_level(LOG_ERROR);
}
AfterEach(BrowserMenu) {}

Ensure(BrowserMenu, can_run_empty_test) {
}
