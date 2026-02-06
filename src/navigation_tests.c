#include "navigation.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "commons.mock"
#include "cxref.mock"           /* Just for refreshStaleReferencesInSession */
#include "editorbuffer.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "session.mock"


Describe(Navigation);
BeforeEach(Navigation) {}
AfterEach(Navigation) {}

Ensure(Navigation, can_run_empty_test) {
}
