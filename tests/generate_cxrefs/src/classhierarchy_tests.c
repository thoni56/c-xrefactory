#include <cgreen/cgreen.h>

#include "classhierarchy.h"

#include "log.h"

#include "commons.mock"
#include "cxref.mock"
#include "filetable.mock"
#include "globals.mock"
#include "menu.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "type.mock"


Describe(ClassHierarchy);
BeforeEach(ClassHierarchy) {
    log_set_level(LOG_ERROR);
}
AfterEach(ClassHierarchy) {}

Ensure(ClassHierarchy, can_run_emtpy_test) {}
