#include <cgreen/cgreen.h>

#include "init.h"

#include "log.h"

#include "globals.mock"
#include "classfilereader.mock"
#include "options.mock"
#include "symboltable.mock"
#include "type.mock"
#include "typemodifier.mock"
#include "symbol.mock"
#include "commons.mock"
#include "misc.mock"


Describe(Init);
BeforeEach(Init) {
    log_set_level(LOG_ERROR);
}
AfterEach(Init) {}

Ensure(Init, can_run_empty_test) {
}
