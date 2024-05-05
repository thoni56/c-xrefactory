#include <cgreen/cgreen.h>

#include "menu.h"

#include "classhierarchy.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "filetable.mock"
#include "globals.mock"
#include "log.h"
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
