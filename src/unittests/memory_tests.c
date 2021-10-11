#include <cgreen/cgreen.h>

#include "memory.h"
#include "log.h"

#include "globals.mock"
#include "commons.mock"           /* For fatalError() */


Describe(Memory);
BeforeEach(Memory) {
    log_set_level(LOG_ERROR);
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

static int overflowRequest = 0;
static bool overflowHandler(int n) {
    overflowRequest = n;
    return true;
}

Ensure(Memory, can_allocate_direct_memory) {
    char *character = NULL;
    Memory memory;

    initMemory((&memory), overflowHandler, SIZE_opiMemory);
    DM_ALLOC((&memory), character, char);
    assert_that(character, is_not_null);
}

Ensure(Memory, will_extend_direct_memory_when_next_allocation_will_fill_up) {
    char *character = NULL;
    Memory memory;

    /* NOTE: This test seems to indicate that overflow happens when there is one byte left */
    if (!setjmp(memoryResizeJumpTarget)) {
        initMemory((&memory), overflowHandler, 10);
        DM_ALLOCC((&memory), character, 9, char);
    } else
        fail_test("unexpected memory resize");
    assert_that(overflowRequest, is_equal_to(0));
    assert_that(character, is_not_null);

    if (!setjmp(memoryResizeJumpTarget)) {
        DM_ALLOC((&memory), character, char);
    }
    assert_that(overflowRequest, is_greater_than(0));
}

Ensure(Memory, has_functions_for_DM_macros) {
    Memory memory;
    memory.index = 42;

    dm_init(&memory);
    assert_that(memory.index, is_equal_to(0));
}
