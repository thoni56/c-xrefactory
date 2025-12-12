#include <cgreen/cgreen.h>

#include "memory.h"
#include "xref.h"

#include "log.h"

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
#include "referenceableitemtable.mock"
#include "reference.mock"
#include "yylex.mock"


Describe(Xref);
BeforeEach(Xref) {
    options.serverOperation = OLO_NOOP;
    log_set_level(LOG_ERROR);
}
AfterEach(Xref) {}

Ensure(Xref, mainCallXref_without_input_files_gives_error_message) {
    expect(getNextScheduledFile, will_return(NULL));
    expect(errorMessage);

    initCxMemory(10000);

    callXref(0, NULL, true);
}
