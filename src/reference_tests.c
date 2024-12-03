#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "reference.h"

#include "options.mock"
#include "misc.mock"
#include "commons.mock"
#include "filetable.mock"


Describe(Reference);
BeforeEach(Reference) {}
AfterEach(Reference) {}


Ensure(Reference, can_allocate_and_free_a_reference) {
    Reference *reference = newReference((Position){0,0,0}, UsageNone, NULL);
    free(reference);
}
