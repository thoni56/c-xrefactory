#include <cgreen/cgreen.h>

#include "memory.h"
#include "log.h"

#include "globals.mock"
#include "commons.mock"           /* For fatalError() */
#include "cxref.mock"             /* For freeOldestOlcx() */


static bool fatalErrorAllowed = false;
static bool myFatalErrorCalled = false;
static void myFatalError(int errCode, char *mess, int exitStatus) {
    if (!fatalErrorAllowed)
        fail_test("FatalError() called");
    myFatalErrorCalled = true;
}


Describe(Memory);
BeforeEach(Memory) {
    log_set_level(LOG_ERROR);
    stackMemoryInit();
    memoryUseFunctionForFatalError(myFatalError);
}
AfterEach(Memory) {}


Ensure(Memory, calls_fatalError_on_out_of_memory) {
    void *allocatedMemory = &allocatedMemory; /* Point to something initially */

    fatalErrorAllowed = true;
    while(!myFatalErrorCalled && allocatedMemory != NULL)
        allocatedMemory = stackMemoryAlloc(5);

    if (!myFatalErrorCalled)
        fail_test("fatalError() not called");
    else
        pass_test();
}

Ensure(Memory, can_begin_and_end_block) {
    FreeTrail freeTrail;

    stackMemoryInit();
    assert_that(currentBlock->outerBlock, is_null);

    currentBlock->trail = &freeTrail; /* "Random" pointer to be able to figure out what happens with it... */

    beginBlock();
    assert_that(currentBlock->outerBlock, is_equal_to(&workMemory[sizeof(CodeBlock)]));
    assert_that(currentBlock->outerBlock, is_not_null);
    assert_that(currentBlock->outerBlock->outerBlock, is_null);
    assert_that(currentBlock->trail, is_equal_to(currentBlock->outerBlock->trail));
    assert_that(currentBlock->trail, is_equal_to(&freeTrail));
    assert_that(currentBlock->outerBlock->trail, is_equal_to(&freeTrail));

    endBlock();
    assert_that(currentBlock->outerBlock, is_null);
}

Ensure(Memory, can_calculate_nesting_level) {
    assert_that(nestingLevel(), is_equal_to(0));
    beginBlock();
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

Ensure(Memory, has_functions_that_can_replace_DM_macros) {
    Memory memory;
    Memory *variablep = NULL;

    memory.index = 42;
    dm_init(&memory, "Memory");
    assert_that(memory.index, is_equal_to(0));
    assert_that(memory.name, is_equal_to_string("Memory"));

    /* Setting this in dm_init() does not work for some reason... */
    memory.overflowHandler = NULL;

    /* This will probably trigger overflow handling so... */
    fatalErrorAllowed = true;
    variablep = dm_allocc(&memory, 1, sizeof(*variablep));
    assert_that(variablep, is_not_null);
    /* TODO This assumes that alignment is initially correct */
    assert_that(memory.index, is_equal_to(sizeof(*variablep)));
}

Ensure(Memory, can_handle_olcx_memory) {
    char *pointer;
    olcxMemoryAllocatedBytes = 42;

    olcx_memory_init();
    assert_that(olcxMemoryAllocatedBytes, is_equal_to(0));

    pointer = NULL;
    OLCX_ALLOCC(pointer, 1, char);
    assert_that(pointer, is_not_null);

    pointer = NULL;
    OLCX_ALLOC(pointer, char);
    assert_that(pointer, is_not_null);
}
