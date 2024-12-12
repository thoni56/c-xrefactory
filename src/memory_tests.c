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

Ensure(Memory, can_resize_cxmemory_using_overflowhandler) {
    assert_that(cxMemory, is_null);
    cxMemoryOverflowHandler(1);
    assert_that(cxMemory->area, is_not_null);
    assert_that(cxMemory->index, is_equal_to(0));
    assert_that(cxMemory->size, is_equal_to(CX_MEMORY_CHUNK_SIZE));
}

Ensure(Memory, can_replicate_main_initialisation_sequence) {
    if (setjmp(memoryResizeJumpTarget) != 0) {
    }
    cxMemoryOverflowHandler(1);
    char *tempAllocated = cxAlloc(CX_MEMORY_CHUNK_SIZE);
    cxFreeUntil(tempAllocated);
}

Ensure(Memory, can_allocate_dynamic_memory_with_overflow_handler) {
    char  *character = NULL;
    Memory memory = {};

    memoryInit(&memory, "memory", overflowHandler, SIZE_optMemory);
    character = memoryAlloc(&memory, sizeof(char));
    assert_that(character, is_not_null);
}

Ensure(Memory, will_extend_direct_memory_when_next_allocation_will_fill_up) {
    char  *character = NULL;
    Memory memory = {};

    if (!setjmp(memoryResizeJumpTarget)) {
        memoryInit(&memory, "", overflowHandler, 10);
        character = memoryAllocc(&memory, 9, sizeof(char));
    } else
        fail_test("unexpected memory resize");
    assert_that(overflowRequest, is_equal_to(0));
    assert_that(character, is_not_null);

    if (!setjmp(memoryResizeJumpTarget)) {
        character = memoryAllocc(&memory, 2, sizeof(char));
    }
    assert_that(overflowRequest, is_greater_than(0));
}


#define SIZE_testMemory 12

Memory testMemory;

Ensure(Memory, can_allocate_with_new_sm_memory) {
    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    char *pointer = memoryAllocc(&testMemory, 1, sizeof(char));
    assert_that(pointer, is_not_null);
    assert_that(pointer, is_equal_to(testMemory.area)); /* First allocated item in testMemory */

    /* Allocate more that size renders fatalError() */
    fatalErrorAllowed = true;
    pointer = memoryAllocc(&testMemory, SIZE_testMemory + 1, sizeof(char));
    assert_that(fatalErrorCalled);
}

Ensure(Memory, will_fatal_on_overflow_in_new_sm_memory) {
    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    /* Allocate more that size renders fatalError() */
    fatalErrorAllowed = true;
    memoryAllocc(&testMemory, SIZE_testMemory + 1, sizeof(char));
    assert_that(fatalErrorCalled);
}

Ensure(Memory, can_free_until_in_new_sm_memory) {
    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = memoryAlloc(&testMemory, 2);
    assert_that(testMemory.index, is_not_equal_to(0));

    memoryAlloc(&testMemory, 4);

    memoryFreeUntil(&testMemory, pointer1);
    assert_that(testMemory.index, is_equal_to(0));
}

Ensure(Memory, can_realloc_in_new_sm_memory) {
    int initialSize = 2;
    int newSize = 4;

    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = memoryAlloc(&testMemory, initialSize);
    assert_that(testMemory.index, is_equal_to(initialSize));

    void *pointer2 = smRealloc(&testMemory, pointer1, initialSize, newSize);
    assert_that(pointer2, is_equal_to(pointer1));
    assert_that(testMemory.index, is_equal_to(newSize));
}

Ensure(Memory, will_fatal_if_freeing_not_in_memory) {
    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    void *pointer = &testMemory.area+1;

    internalCheckFailAllowed = true;
    memoryFreeUntil(&testMemory, pointer);

    assert_that(internalCheckFailCalled);
}

Ensure(Memory, will_fatal_if_reallocing_not_last_allocated) {
    memoryInit(&testMemory, "", NULL, SIZE_testMemory);

    void *pointer = memoryAlloc(&testMemory, 5);

    internalCheckFailAllowed = true;
    smRealloc(&testMemory, pointer+1, 5, 6);

    assert_that(internalCheckFailCalled);
}
