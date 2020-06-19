#include <cgreen/cgreen.h>

#include "lexembuffer.h"

Describe(LexemBuffer);
BeforeEach(LexemBuffer) {}
AfterEach(LexemBuffer) {}


Ensure(LexemBuffer, can_pass_an_empty_test) {
    pass_test();
}
