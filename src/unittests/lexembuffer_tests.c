#include <cgreen/cgreen.h>

#include "lexem.h"
#include "lexembuffer.h"

#include "zlib.mock"
#include "fileio.mock"
#include "commons.mock"


Describe(LexemBuffer);
BeforeEach(LexemBuffer) {}
AfterEach(LexemBuffer) {}


Ensure(LexemBuffer, can_put_and_get_a_char) {
    LexemBuffer buffer;
    char ch;
    char *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexChar('x', &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.chars;
    ch = getLexChar(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(ch, is_equal_to('x'));
}

Ensure(LexemBuffer, can_put_and_get_a_short) {
    LexemBuffer buffer;
    int shortValue;
    char *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexShort(433, &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.chars;
    shortValue = getLexShort(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(shortValue, is_equal_to(433));
}

Ensure(LexemBuffer, can_put_and_get_a_token) {
    LexemBuffer buffer;
    Lexem lexem;
    char *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexToken(DOUBLE_CONSTANT, &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.chars;
    lexem = getLexToken(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(lexem, is_equal_to(DOUBLE_CONSTANT));
}
