#include <cgreen/cgreen.h>

#include "classfilereader.h"
#include "log.h"

#include "globals.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "filetable.mock"
#include "options.mock"
#include "yylex.mock"
#include "editor.mock"
#include "characterreader.mock"
#include "fileio.mock"
#include "symbol.mock"
#include "classcaster.mock"
#include "jsemact.mock"
#include "filedescriptor.mock"
#include "typemodifier.mock"
#include "cxfile.mock"


Describe(Classfilereader);
BeforeEach(Classfilereader) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(Classfilereader) {}

Ensure(Classfilereader, can_run_empty_test) {
}
