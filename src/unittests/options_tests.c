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

Ensure(Options, will_return_true_if_package_structure_exists) {
    javaSourcePaths = ".";
    expect(dirExists, when(fullPath, is_equal_to_string("./org/existant")),
           will_return(true));
    expect(dirInputFile);

    assert_that(packageOnCommandLine("org.existant"));
}
