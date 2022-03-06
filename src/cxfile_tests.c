#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "cxfile.h"
#include "log.h"

#include "globals.mock"
#include "misc.mock"
#include "utils.mock"
#include "options.mock"
#include "commons.mock"

#include "olcxtab.mock"
#include "cxref.mock"
#include "reftab.mock"
#include "filetable.mock"
#include "yylex.mock"           /* For addFiletabItem */
#include "editor.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "fileio.mock"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
    options.referenceFileCount = 1;
    assert_that(cxFileHashNumber(NULL), is_equal_to(0));
}

xEnsure(CxFile, can_do_normal_scan) {
    options.cxrefsLocation = "./CXrefs";

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return((FILE*) 4654654645));
    always_expect(skipWhiteSpace);

    normalScanReferenceFile("/XFiles");
}
