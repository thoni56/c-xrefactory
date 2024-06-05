#include <cgreen/cgreen.h>

#include "caching.h"

#include "commons.mock"
#include "counters.mock"
#include "cxref.mock" /* For freeOldestOlcx() */
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "input.mock"
#include "options.mock"
#include "reftab.mock"
#include "symboltable.mock"
#include "yylex.mock"

Describe(Caching);
BeforeEach(Caching) {}
AfterEach(Caching) {}

Ensure(Caching, can_run_empty_test) {}
