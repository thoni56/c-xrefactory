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


Describe(Menu);
BeforeEach(Menu) {
    log_set_level(LOG_ERROR);
}
AfterEach(Menu) {}

Ensure(Menu, can_run_empty_test) {
}
