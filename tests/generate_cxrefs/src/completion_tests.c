#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "completion.h"

#include "filetable.mock"
#include "cxfile.mock"
#include "options.mock"
#include "reference.mock"
#include "symbol.mock"


Describe(Completion);
BeforeEach(Completion) {}
AfterEach(Completion) {}

Ensure(Completion, can_compile_an_empty_test) {
    olcxFreeCompletions(NULL);
}
