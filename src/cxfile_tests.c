#include <cgreen/cgreen.h>

#include "cxfile.h"
#include "log.h"

#include "globals.mock"
#include "olcxtab.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "reftab.mock"
#include "filetable.mock"
#include "options.mock"
#include "yylex.mock"
#include "lexer.mock"
#include "editor.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "fileio.mock"
#include "utils.mock"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
    options.referenceFileCount = 1;
    assert_that(cxFileHashNumber(NULL), is_equal_to(0));
}
