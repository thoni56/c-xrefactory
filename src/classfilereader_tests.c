#include <cgreen/cgreen.h>

#include "classfilereader.h"
#include "log.h"

#include "characterreader.mock"
#include "classcaster.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "symbol.mock"
#include "typemodifier.mock"
#include "yylex.mock"

Describe(Classfilereader);
BeforeEach(Classfilereader) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(Classfilereader) {}

Ensure(Classfilereader, can_run_empty_test) {}
