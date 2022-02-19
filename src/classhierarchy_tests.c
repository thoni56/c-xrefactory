#include <cgreen/cgreen.h>

#include "classhierarchy.h"
#include "log.h"
#include "protocol.h"

#include "options.mock"
#include "globals.mock"
#include "filetable.mock"
#include "type.mock"
#include "misc.mock"
#include "cxref.mock"
#include "ppc.mock"
#include "commons.mock"


Describe(ClassHierarchy);
BeforeEach(ClassHierarchy) {
    log_set_level(LOG_ERROR);
}
AfterEach(ClassHierarchy) {}


Ensure(ClassHierarchy, can_run_emtpy_test) {
}
