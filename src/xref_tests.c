#include <cgreen/cgreen.h>

#include "xref.h"

#include "log.h"

#include "caching.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "main.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "progress.mock"
#include "reftab.mock"
#include "reference.mock"


Describe(Xref);
BeforeEach(Xref) {
    options.serverOperation = OLO_NOOP;
    log_set_level(LOG_ERROR);
}
AfterEach(Xref) {}

Ensure(Xref, mainCallXref_without_input_files_gives_error_message) {
    expect(getNextScheduledFile, will_return(NULL));
    expect(errorMessage);

    cxMemoryOverflowHandler(0); /* Implicitly allocate and init cxMemory */

    callXref(0, NULL, true);
}
