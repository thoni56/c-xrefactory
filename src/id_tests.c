#include <cgreen/cgreen.h>

#include "id.h"

/* Dependencies: */
#include "cxref.mock"
#include "globals.mock"
#include "commons.mock"
#include "stackmemory.h"


Describe(Id);
BeforeEach(Id) {
    initOuterCodeBlock();
}
AfterEach(Id) {}

Ensure(Id, can_run_empty_test) {
}
