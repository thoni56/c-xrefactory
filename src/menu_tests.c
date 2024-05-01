#include <cgreen/cgreen.h>

#include "log.h"

#include "menu.h"

#include "classhierarchy.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "globals.mock"
#include "options.mock"
#include "ppc.mock"
#include "reference.mock"
#include "filetable.mock"


Describe(Menu);
BeforeEach(Menu) {
    log_set_level(LOG_ERROR);
}
AfterEach(Menu) {}

Ensure(Menu, can_run_empty_test) {
}
