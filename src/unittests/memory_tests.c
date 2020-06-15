#include <cgreen/cgreen.h>

#include "memory.h"
#include "globals.mock"


Describe(Memory);
BeforeEach(Memory) {
    stackMemoryInit();
}
AfterEach(Memory) {}

static bool myFatalErrorCalled = false;
static void myFatalError(int errCode, char *mess, int exitStatus) {
    myFatalErrorCalled = true;
}

Ensure(Memory, calls_fatalError_on_out_of_memory) {
    void *allocatedMemory = &allocatedMemory; /* Point to something initially */

    memoryUseFunctionForFatalError(myFatalError);

    while(!myFatalErrorCalled && allocatedMemory != NULL)
        allocatedMemory = stackMemoryAlloc(5);

    if (!myFatalErrorCalled)
        fail_test("fatalError() not called");
    else
        pass_test();
}

Ensure(Memory, can_calculate_nesting_level) {
    assert_that(nestingLevel(), is_equal_to(0));
    stackMemoryBlockStart();
    assert_that(nestingLevel(), is_equal_to(1));
}
