#include <cgreen/cgreen.h>

#include "options.h"

#include "globals.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "main.mock"
#include "editor.mock"
#include "fileio.mock"


Describe(Options);
BeforeEach(Options) {}
AfterEach(Options) {}


Ensure(Options, will_return_false_if_package_structure_does_not_exist) {
    assert_that(!packageOnCommandLine("org.nonexistant"));
}
