#include <cgreen/cgreen.h>

#include "lexem.h"
#include "lexembuffer.h"
#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "zlib.mock"

Describe(LexemBuffer);
BeforeEach(LexemBuffer) {
    log_set_level(LOG_ERROR);
}
AfterEach(LexemBuffer) {}

Ensure(LexemBuffer, can_put_and_get_a_char) {
    LexemBuffer buffer;
    char        ch;
    char       *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexChar('x', &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.lexemStream;
    ch          = getLexChar(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(ch, is_equal_to('x'));
}

Ensure(LexemBuffer, can_put_and_get_a_short) {
    LexemBuffer buffer;
    int         shortValue;
    char       *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexShort(433, &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.lexemStream;
    shortValue  = getLexShort(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(shortValue, is_equal_to(433));
}

Ensure(LexemBuffer, can_put_and_get_a_token) {
    LexemBuffer buffer;
    Lexem       lexem;
    char       *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexToken(DOUBLE_CONSTANT, &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.lexemStream;
    lexem       = getLexToken(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(lexem, is_equal_to(DOUBLE_CONSTANT));
}

Ensure(LexemBuffer, can_put_and_get_an_int) {
    LexemBuffer buffer;
    int         integer;
    char       *next_after_put;

    initLexemBuffer(&buffer, NULL);

    putLexInt(34581, &buffer.next);
    next_after_put = buffer.next;

    buffer.next = buffer.lexemStream;
    integer     = getLexInt(&buffer.next);

    assert_that(buffer.next, is_equal_to(next_after_put));
    assert_that(integer, is_equal_to(34581));
}

Ensure(LexemBuffer, can_put_and_get_a_compacted_int) {
    LexemBuffer buffer;
    int         integer;
    char       *next_after_put;

    initLexemBuffer(&buffer, NULL);

    /* Test values across the 128 boundry, 1 or 2 "slots" */
    for (int i = 0; i < 150; i++) {
        buffer.next = buffer.lexemStream;

        putLexCompacted(i, &buffer.next);
        next_after_put = buffer.next;

        buffer.next = buffer.lexemStream;
        integer     = getLexCompacted(&buffer.next);

        assert_that(buffer.next, is_equal_to(next_after_put));
        assert_that(integer, is_equal_to(i));
    }

    /* Test values across the 16384 boundry, 2 or 3 "slots" */
    for (int i = 16300; i < 16500; i++) {
        buffer.next = buffer.lexemStream;

        putLexCompacted(i, &buffer.next);
        next_after_put = buffer.next;

        buffer.next = buffer.lexemStream;
        integer     = getLexCompacted(&buffer.next);

        assert_that(buffer.next, is_equal_to(next_after_put));
        assert_that(integer, is_equal_to(i));
    }
}

Ensure(LexemBuffer, can_peek_next_token) {
    LexemBuffer lb;
    Lexem any_lexem = CHAR_LITERAL;

    initLexemBuffer(&lb, NULL);

    putLexToken(any_lexem, &lb.end);

    assert_that(nextLexToken(&lb.next), is_equal_to(any_lexem));
    assert_that(lb.next, is_equal_to(&lb.lexemStream)); /* Should not have moved */
}

Ensure(LexemBuffer, can_put_and_get_lines) {
    LexemBuffer lb;
    char *pointer_after_put = NULL;

    initLexemBuffer(&lb, NULL);

    putLexLines(13, &lb.end);
    pointer_after_put = lb.end;

    assert_that(getLexToken(&lb.next), is_equal_to(LINE_TOKEN));
    assert_that(getLexToken(&lb.next), is_equal_to(13));
    assert_that(lb.next, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_put_and_get_position) {
    LexemBuffer lb;
    char    *pointer_after_put = NULL;
    Position initial_position  = {41, 42, 43};
    Position read_position;

    initLexemBuffer(&lb, NULL);

    putLexPosition(initial_position.file, initial_position.line, initial_position.col, &lb.end);
    pointer_after_put = lb.end;

    read_position = getLexPosition(&lb.next);
    assert_that(positionsAreEqual(read_position, initial_position));
    assert_that(lb.next, is_equal_to(pointer_after_put));
}
