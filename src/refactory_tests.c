#include <cgreen/cgreen.h>

#include "refactory.h"

#include "log.h"
#include "memory.h"

#include "classhierarchy.mock"
#include "commons.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "main.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "progress.mock"
#include "reftab.mock"
#include "server.mock"
#include "xref.mock"


Describe(Refactory);
BeforeEach(Refactory) {
    log_set_level(LOG_ERROR);
}
AfterEach(Refactory) {}

Ensure(Refactory, can_run_empty_test) {}
