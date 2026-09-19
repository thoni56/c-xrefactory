#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "counters.h"

#include "semact.mock"


Describe(Counters);
BeforeEach(Counters) {}
AfterEach(Counters) {}

Ensure(Counters, can_run_empty_test) {
}
