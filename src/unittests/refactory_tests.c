#include <cgreen/cgreen.h>

#include "refactory.h"
#include "log.h"

#include "commons.mock"
#include "globals.mock"
#include "editor.mock"
#include "ppc.mock"
#include "options.mock"
#include "classhierarchy.mock"
#include "main.mock"
#include "cxref.mock"
#include "misc.mock"
#include "olcxtab.mock"
#include "reftab.mock"
#include "filetable.mock"
#include "cxfile.mock"
#include "memory.mock"
#include "complete.mock"
#include "jsemact.mock"



Describe(Refactory);
BeforeEach(Refactory) {
    log_set_level(LOG_ERROR);
}
AfterEach(Refactory) {}

Ensure(Refactory, can_run_empty_test) {
}
