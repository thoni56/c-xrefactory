#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "misc.h"

#include "globals.mock"
#include "caching.mock"
#include "filetab.mock"
#include "jsemact.mock"
#include "classfilereader.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "yylex.mock"
#include "memory.mock"
#include "commons.mock"
#include "editor.mock"


Describe(Misc);
BeforeEach(Misc) {}
AfterEach(Misc) {}


Ensure(Misc, can_run_an_empty_test) {
}
