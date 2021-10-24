#include <cgreen/cgreen.h>

#include "../caching.h"

#include "yylex.mock"
#include "globals.mock"
#include "options.mock"
#include "reftab.mock"
#include "javafqttab.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "symboltable.mock"
#include "jsemact.mock"
#include "editor.mock"
#include "commons.mock"
#include "cxref.mock"           /* For freeOldestOlcx() */



Describe(Caching);
BeforeEach(Caching) {}
AfterEach(Caching) {}


Ensure(Caching, can_run_empty_test) {
}
