#include <cgreen/cgreen.h>

#include "extract.h"

#include "log.h"
#include "stackmemory.h"

/* Dependencies: */
#include "caching.mock"
#include "counters.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "refactory.mock"
#include "reftab.mock"
#include "semact.mock"
#include "symbol.mock"
#include "symboltable.mock"


void myFatalError(int errCode, char *mess, int exitStatus, char *file, int line) {
    fail_test("Fatal Error");
}

void myInternalCheckFail(char *expr, char *file, int line) {
    fail_test("Internal Check Failed");
}

void myError(int errCode, char *mess) {
    fail_test("Error");
}

Describe(Extract);
BeforeEach(Extract) {
    log_set_level(LOG_ERROR);
    setErrorHandlerForStackMemory(myError);
    setInternalCheckFailHandlerForMemory(myInternalCheckFail);
    setFatalErrorHandlerForMemory(myFatalError);
    options.cxMemoryFactor = 2;
    if (!cxMemoryOverflowHandler(1))
        fail_test("cxMemoryOverflowHandler() returned false");
}
AfterEach(Extract) {}


Ensure(Extract, can_run_empty_unittest) {
}
