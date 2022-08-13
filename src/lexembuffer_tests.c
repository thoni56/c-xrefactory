#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

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
    LexemBuffer lb;
    char        ch;
    char       *next_after_put;

    initLexemBuffer(&lb, NULL);

    putLexChar('x', &lb.next);
    next_after_put = lb.next;

    lb.next = lb.lexemStream;
    ch      = getLexChar(&lb.next);

    assert_that(lb.next, is_equal_to(next_after_put));
    assert_that(ch, is_equal_to('x'));
}

Ensure(LexemBuffer, can_put_and_get_a_short) {
    LexemBuffer lb;
    int         shortValue;
    char       *next_after_put;

    initLexemBuffer(&lb, NULL);

    putLexShort(433, &lb.next);
    next_after_put = lb.next;

    lb.next = lb.lexemStream;
    shortValue  = getLexShort(&lb.next);

    assert_that(lb.next, is_equal_to(next_after_put));
    assert_that(shortValue, is_equal_to(433));
}

Ensure(LexemBuffer, can_put_and_get_a_token) {
    LexemBuffer lb;
    Lexem       lexem;
    char       *next_after_put;

    initLexemBuffer(&lb, NULL);

    putLexToken(DOUBLE_CONSTANT, &lb.next);
    next_after_put = lb.next;

    lb.next = lb.lexemStream;
    lexem       = getLexToken(&lb.next);

    assert_that(lb.next, is_equal_to(next_after_put));
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

    putLexLines(13, &lb);
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

    putLexPositionFields(initial_position.file, initial_position.line, initial_position.col, &lb.end);
    pointer_after_put = lb.end;

    read_position = getLexPosition(&lb.next);
    assert_that(positionsAreEqual(read_position, initial_position));
    assert_that(lb.next, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_backpatch_lexem) {
    LexemBuffer lb;
    Lexem backpatched_lexem = STRING_LITERAL;

    initLexemBuffer(&lb, NULL);

    int backpatchIndex = getCurrentLexemIndexForBackpatching(&lb);
    putLexToken(CPP_LINE, &lb.end);

    char *lb_end = lb.end;
    char *lb_next = lb.next;

    backpatchLexem(&lb, backpatchIndex, backpatched_lexem);

    assert_that(getLexemAt(&lb, backpatchIndex), is_equal_to(backpatched_lexem));

    /* lb pointers should not have moved */
    assert_that(lb_end, is_equal_to(lb.end));
    assert_that(lb_next, is_equal_to(lb.next));
}

Ensure(LexemBuffer, can_set_next_write_position) {
    LexemBuffer lb;

    initLexemBuffer(&lb, NULL);

    putLexToken(IDENTIFIER, &lb.end);
    assert_that(lb.end, is_not_equal_to(lb.lexemStream));

    setLexemStreamEnd(&lb, 0);
    assert_that(lb.end, is_equal_to(lb.lexemStream));
}
