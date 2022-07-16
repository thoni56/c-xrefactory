#include <cgreen/cgreen.h>

#include "xref.h"

#include "log.h"

#include "cxref.mock"
#include "cxfile.mock"
#include "reftab.mock"
#include "caching.mock"
#include "characterreader.mock"
#include "filedescriptor.mock"
#include "options.mock"
#include "main.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "editor.mock"
#include "misc.mock"
#include "commons.mock"
#include "globals.mock"
#include "filetable.mock"


Describe(Xref);
BeforeEach(Xref) {
    log_set_level(LOG_ERROR);
}
AfterEach(Xref) {}

Ensure(Xref, mainCallXref_without_input_files_gives_error_message) {
    expect(getNextScheduledFile, will_return(NULL));
    expect(errorMessage);

    cxMemoryOverflowHandler(0); /* Implicitly allocate and init cxMemory */

    mainCallXref(0, NULL);
}
