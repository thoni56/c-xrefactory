#include <cgreen/cgreen.h>

#include "extract.h"

#include "log.h"


/* Dependencies: */
#include "commons.mock"
#include "cxref.mock"
#include "filedescriptor.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "reftab.mock"
#include "semact.mock"
#include "symbol.mock"
#include "symboltable.mock"


Describe(Extract);
BeforeEach(Extract) {
    log_set_level(LOG_ERROR);
}
AfterEach(Extract) {}


Ensure(Extract, can_run_empty_unittest) {
}
