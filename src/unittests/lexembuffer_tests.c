#include <cgreen/cgreen.h>

#include "lexem.h"
#include "lexembuffer.h"

#include "zlib.mock"
#include "fileio.mock"
#include "commons.mock"


Describe(LexemBuffer);
BeforeEach(LexemBuffer) {}
AfterEach(LexemBuffer) {}


Ensure(LexemBuffer, can_put_and_get_a_token) {
    LexemBuffer buffer;
    Lexem lexem;
    char *next_after_put;

    initLexemBuffer(&buffer, NULL);

    PutLexToken(DOUBLE_CONSTANT, buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.chars;
    GetLexToken(lexem, buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(lexem, is_equal_to(DOUBLE_CONSTANT));
}
