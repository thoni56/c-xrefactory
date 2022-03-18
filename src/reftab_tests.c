#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "reftab.h"
#include "log.h"

#include "cxref.mock"


Describe(ReferenceTable);
BeforeEach(ReferenceTable) {
    log_set_level(LOG_ERROR);
    initReferenceTable();
}
AfterEach(ReferenceTable) {}


Ensure(ReferenceTable, can_run_empty_test) {
}
