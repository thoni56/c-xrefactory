#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "commons.h"
#include "constants.h"
#include "log.h"
#include "memory.h"

#include "commons.mock" /* For fatalError() */
#include "cxref.mock"   /* For freeOldestOlcx() */
#include "globals.mock"

static bool fatalErrorAllowed = false;
static bool fatalErrorCalled  = false;
static void myFatalError(int errCode, char *mess, int exitStatus, char *file, int line) {
    if (!fatalErrorAllowed)
        fail_test("FatalError() called");
    fatalErrorCalled = true;
}

static bool internalCheckFailAllowed = false;
static bool internalCheckFailCalled  = false;
static void myInternalCheckFailed(char *expr, char *file, int line) {
    if (!internalCheckFailAllowed)
        internalCheckFail(expr, file, line);
    internalCheckFailCalled = true;
}

Describe(Memory);
BeforeEach(Memory) {
    log_set_level(LOG_ERROR);
    setFatalErrorHandlerForMemory(myFatalError);
    setInternalCheckFailHandlerForMemory(myInternalCheckFailed);
}
AfterEach(Memory) {}

static int  overflowRequest = 0;
static bool overflowHandler(int n) {
    overflowRequest = n;
    return true;
}

Ensure(Memory, can_allocate_dynamic_memory_with_overflow_handler) {
    char  *character = NULL;
    Memory memory;

    initMemory(&memory, "memory", overflowHandler, SIZE_optMemory);
    character = dm_alloc(&memory, sizeof(char));
    assert_that(character, is_not_null);
}

Ensure(Memory, will_extend_direct_memory_when_next_allocation_will_fill_up) {
    char  *character = NULL;
    Memory memory;

    /* NOTE: This test seems to indicate that overflow happens when there is one byte left */
    if (!setjmp(memoryResizeJumpTarget)) {
        initMemory(&memory, "", overflowHandler, 10);
        character = dm_allocc(&memory, 9, sizeof(char));
    } else
        fail_test("unexpected memory resize");
    assert_that(overflowRequest, is_equal_to(0));
    assert_that(character, is_not_null);

    if (!setjmp(memoryResizeJumpTarget)) {
        character = dm_alloc(&memory, sizeof(char));
    }
    assert_that(overflowRequest, is_greater_than(0));
}

Ensure(Memory, has_functions_that_can_replace_DM_macros) {
    Memory  memory;
    Memory *variablep = NULL;

    memory.index = 42;
    dm_init(&memory, "Memory");
    assert_that(memory.index, is_equal_to(0));
    assert_that(memory.name, is_equal_to_string("Memory"));

    /* Setting this in dm_init() does not work for some reason... */
    memory.overflowHandler = NULL;

    /* This will probably trigger overflow handling so... */
    fatalErrorAllowed = true;
    variablep         = dm_allocc(&memory, 1, sizeof(*variablep));
    assert_that(variablep, is_not_null);
    /* TODO This assumes that alignment is initially correct */
    assert_that(memory.index, is_equal_to(sizeof(*variablep)));
}


#define SIZE_testMemory 12

Memory2 testMemory2;

Ensure(Memory, can_allocate_with_new_sm_memory) {
    smInit(&testMemory2, "", SIZE_testMemory);

    char *pointer = smAllocc(&testMemory2, 1, sizeof(char));
    assert_that(pointer, is_not_null);
    assert_that(pointer, is_equal_to(testMemory2.area)); /* First allocated item in testMemory */

    /* Allocate more that size renders fatalError() */
    fatalErrorAllowed = true;
    pointer = smAllocc(&testMemory2, SIZE_testMemory + 1, sizeof(char));
    assert_that(fatalErrorCalled);
}

Ensure(Memory, will_fatal_on_overflow_in_new_sm_memory) {
    smInit(&testMemory2, "", SIZE_testMemory);

    /* Allocate more that size renders fatalError() */
    fatalErrorAllowed = true;
    smAllocc(&testMemory2, SIZE_testMemory + 1, sizeof(char));
    assert_that(fatalErrorCalled);
}

Ensure(Memory, can_free_until_in_new_sm_memory) {
    smInit(&testMemory2, "", SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = smAlloc(&testMemory2, 2);
    assert_that(testMemory2.index, is_not_equal_to(0));

    smAlloc(&testMemory2, 4);

    smFreeUntil(&testMemory2, pointer1);
    assert_that(testMemory2.index, is_equal_to(0));
}

Ensure(Memory, can_realloc_in_new_sm_memory) {
    int initialSize = 2;
    int newSize = 4;

    smInit(&testMemory2, "", SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = smAlloc(&testMemory2, initialSize);
    assert_that(testMemory2.index, is_equal_to(initialSize));

    void *pointer2 = smRealloc(&testMemory2, pointer1, initialSize, newSize);
    assert_that(pointer2, is_equal_to(pointer1));
    assert_that(testMemory2.index, is_equal_to(newSize));
}

Ensure(Memory, will_fatal_if_freeing_not_in_memory) {
    smInit(&testMemory2, "", SIZE_testMemory);

    void *pointer = &testMemory2.area+1;

    internalCheckFailAllowed = true;
    smFreeUntil(&testMemory2, pointer);

    assert_that(internalCheckFailCalled);
}

Ensure(Memory, will_fatal_if_reallocing_not_last_allocated) {
    smInit(&testMemory2, "", SIZE_testMemory);

    void *pointer = smAlloc(&testMemory2, 5);

    internalCheckFailAllowed = true;
    smRealloc(&testMemory2, pointer+1, 5, 6);

    assert_that(internalCheckFailCalled);
}
