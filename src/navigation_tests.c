#include "navigation.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "browsingmenu.mock"
#include "commons.mock"
#include "cxref.mock"           /* For recomputeSelectedReferenceable */
#include "editorbuffer.mock"
#include "filetable.mock"
#include "globals.mock"
#include "options.mock"
#include "ppc.mock"
#include "referenceableitemtable.mock"


Describe(Navigation);
BeforeEach(Navigation) {}
AfterEach(Navigation) {}

Ensure(Navigation, can_run_empty_test) {
}
