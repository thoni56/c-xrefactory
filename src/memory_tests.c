#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include <setjmp.h>

#include "commons.h"
#include "memory.h"

#include "commons.mock" /* For fatalError() */
#include "globals.mock"
#include "log.mock"


static bool fatalErrorAllowed = false;
static bool fatalErrorCalled  = false;
static void myFatalError(int errCode, char *mess, int exitStatus, char *file, int line) {
    if (!fatalErrorAllowed)
        fail_test("FatalError() called");
    fatalErrorCalled = true;
}


static jmp_buf internalCheckJmpbuf;
static bool internalCheckFailAllowed = false;
static bool internalCheckFailCalled  = false;
static void myInternalCheckFailed(char *expr, char *file, int line) {
    if (!internalCheckFailAllowed)
        internalCheckFail(expr, file, line);
    internalCheckFailCalled = true;
    longjmp(internalCheckJmpbuf, 1);
}


Describe(Memory);
BeforeEach(Memory) {
    log_set_level(LOG_ERROR);
    setFatalErrorHandlerForMemory(myFatalError);
    setInternalCheckFailHandlerForMemory(myInternalCheckFailed);
}
AfterEach(Memory) {}


#define SIZE_testMemory 12

Memory testMemory;

Ensure(Memory, can_allocate_memory) {
    memoryInit(&testMemory, "", SIZE_testMemory);

    char *pointer = memoryAllocc(&testMemory, 1, sizeof(char));
    assert_that(pointer, is_not_null);
    assert_that(pointer, is_equal_to(testMemory.area)); /* First allocated item in testMemory */
}

Ensure(Memory, will_fatal_on_overflow_in_memory) {
    memoryInit(&testMemory, "", SIZE_testMemory);

    /* Allocate more that size renders fatalError() */
    fatalErrorAllowed = true;
    memoryAllocc(&testMemory, SIZE_testMemory + 1, sizeof(char));
    assert_that(fatalErrorCalled);
}

Ensure(Memory, can_free_until_in_memory) {
    memoryInit(&testMemory, "", SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = memoryAlloc(&testMemory, 2);
    assert_that(testMemory.index, is_not_equal_to(0));

    memoryAlloc(&testMemory, 4);

    memoryFreeUntil(&testMemory, pointer1);
    assert_that(testMemory.index, is_equal_to(0));
}

Ensure(Memory, can_realloc_in_memory) {
    int initialSize = 2;
    int newSize = 4;

    memoryInit(&testMemory, "", SIZE_testMemory);

    /* Allocate some memory */
    void *pointer1 = memoryAlloc(&testMemory, initialSize);
    assert_that(testMemory.index, is_equal_to(initialSize));

    void *pointer2 = memoryRealloc(&testMemory, pointer1, initialSize, newSize);
    assert_that(pointer2, is_equal_to(pointer1));
    assert_that(testMemory.index, is_equal_to(newSize));
}

Ensure(Memory, should_fatal_if_freeing_not_in_memory) {
    memoryInit(&testMemory, "", SIZE_testMemory);

    void *pointer = &testMemory.area+1;

    internalCheckFailAllowed = true;
    if (setjmp(internalCheckJmpbuf) == 0)
        memoryFreeUntil(&testMemory, pointer);

    assert_that(internalCheckFailCalled);
}

Ensure(Memory, should_fatal_if_reallocing_not_last_allocated) {
    memoryInit(&testMemory, "", SIZE_testMemory);

    void *pointer = memoryAlloc(&testMemory, 5);

    fatalErrorAllowed = true;
    internalCheckFailAllowed = true;
    if (setjmp(internalCheckJmpbuf) == 0)
        memoryRealloc(&testMemory, pointer+1, 5, 6);

    assert_that(internalCheckFailCalled);
}
