#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"

#include "stackmemory.h"

#include "memory.h"
#include "commons.mock"


/* Our own fatalError that can be customized */
static bool fatalErrorAllowed = false;
static bool fatalErrorCalled  = false;
static void myFatalError(int errCode, char *mess, int exitStatus, char *file, int line) {
    if (!fatalErrorAllowed)
        fail_test("FatalError() called");
    fatalErrorCalled = true;
}


Describe(StackMemory);
BeforeEach(StackMemory) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();
    fatalErrorAllowed = true;
    setFatalErrorHandlerForMemory(myFatalError);
}
AfterEach(StackMemory) {}

Ensure(StackMemory, calls_fatalError_on_out_of_memory) {
    void *allocatedMemory = &allocatedMemory; /* Point to something initially */

    expect(fatalError);
    while (!fatalErrorCalled && allocatedMemory != NULL)
        allocatedMemory = stackMemoryAlloc(5);
}

Ensure(StackMemory, can_begin_and_end_block) {
    FreeTrail freeTrail;

    initOuterCodeBlock();
    assert_that(currentBlock->outerBlock, is_null);

    currentBlock->trail = &freeTrail; /* "Random" pointer to be able to figure out what happens with it... */

    beginBlock();
    assert_that(currentBlock->outerBlock, is_equal_to(&stackMemory[sizeof(CodeBlock)]));
    assert_that(currentBlock->outerBlock, is_not_null);
    assert_that(currentBlock->outerBlock->outerBlock, is_null);
    assert_that(currentBlock->trail, is_equal_to(currentBlock->outerBlock->trail));
    assert_that(currentBlock->trail, is_equal_to(&freeTrail));
    assert_that(currentBlock->outerBlock->trail, is_equal_to(&freeTrail));

    endBlock();
    assert_that(currentBlock->outerBlock, is_null);
}

Ensure(StackMemory, can_calculate_nesting_level) {
    assert_that(nestingLevel(), is_equal_to(0));
    beginBlock();
    assert_that(nestingLevel(), is_equal_to(1));
}
