#include <cgreen/cgreen.h>

#include "init.h"

#include "log.h"

#include "commons.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "symbol.mock"
#include "symboltable.mock"
#include "type.mock"
#include "typemodifier.mock"

Describe(Init);
BeforeEach(Init) {
    log_set_level(LOG_ERROR);
}
AfterEach(Init) {}

Ensure(Init, can_run_empty_test) {}
