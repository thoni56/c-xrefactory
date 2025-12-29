#include "reference.h"

#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "commons.mock"
#include "filetable.mock"
#include "misc.mock"
#include "options.mock"
#include "parsing.mock"


Describe(ReferenceableItem);
BeforeEach(ReferenceableItem) {}
AfterEach(ReferenceableItem) {}


Ensure(ReferenceableItem, can_allocate_and_free_a_reference) {
    Reference *reference = newReference((Position){0,0,0}, UsageNone, NULL);
    free(reference);
}
