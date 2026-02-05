#include "navigation.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "commons.mock"
#include "filetable.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"


Describe(Navigation);
BeforeEach(Navigation) {}
AfterEach(Navigation) {}

Ensure(Navigation, can_run_empty_test) {
}
