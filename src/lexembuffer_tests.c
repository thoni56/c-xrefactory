#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "lexem.h"
#include "lexembuffer.h"
#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "zlib.mock"


LexemBuffer lb;

Describe(LexemBuffer);
BeforeEach(LexemBuffer) {
    log_set_level(LOG_ERROR);
    initLexemBuffer(&lb, NULL);
}
AfterEach(LexemBuffer) {}

Ensure(LexemBuffer, can_put_and_get_a_char) {
    char        ch;
    char       *expected_next_after_get;

    putLexChar(&lb, 'x');
    expected_next_after_get = lb.end;

    ch = getLexChar(&lb.next);

    assert_that(lb.next, is_equal_to(expected_next_after_get));
    assert_that(ch, is_equal_to('x'));
}

extern void putLexShort(int shortValue, char **writePointer);
extern int getLexShort(char **readPointer);
Ensure(LexemBuffer, can_put_and_get_a_short) {
    int         shortValue;
    char       *expected_next_after_get;

    putLexShort(433, &lb.end);
    expected_next_after_get = lb.end;

    shortValue  = getLexShort(&lb.next);

    assert_that(lb.next, is_equal_to(expected_next_after_get));
    assert_that(shortValue, is_equal_to(433));
}

Ensure(LexemBuffer, can_put_and_get_a_token) {
    Lexem       lexem;
    char       *expected_next_after_get;

    putLexToken(&lb, DOUBLE_CONSTANT);
    expected_next_after_get = lb.end;

    lexem = getLexToken(&lb.next);

    assert_that(lb.next, is_equal_to(expected_next_after_get));
    assert_that(lexem, is_equal_to(DOUBLE_CONSTANT));
}

Ensure(LexemBuffer, can_put_and_get_an_int) {
    int         integer;
    char       *expected_next_after_get;

    putLexInt(34581, &lb.end);
    expected_next_after_get = lb.end;

    integer     = getLexInt(&lb.next);

    assert_that(lb.next, is_equal_to(expected_next_after_get));
    assert_that(integer, is_equal_to(34581));
}

extern void putLexCompacted(int value, char **writePointer);
extern int getLexCompacted(char **readPointer);
Ensure(LexemBuffer, can_put_and_get_a_compacted_int) {
    int         integer;
    char       *expected_next_after_get;

    /* Test values across the 128 boundry, 1 or 2 "slots" */
    for (int i = 0; i < 150; i++) {
        putLexCompacted(i, &lb.end);
        expected_next_after_get = lb.end;

        integer     = getLexCompacted(&lb.next);

        assert_that(lb.next, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }

    /* Test values across the 16384 boundry, 2 or 3 "slots" */
    for (int i = 16300; i < 16500; i++) {
        putLexCompacted(i, &lb.end);
        expected_next_after_get = lb.end;

        integer     = getLexCompacted(&lb.next);

        assert_that(lb.next, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }
}

Ensure(LexemBuffer, can_peek_next_token) {
    Lexem any_lexem = CHAR_LITERAL;

    putLexToken(&lb, any_lexem);

    assert_that(peekLexToken(&lb.next), is_equal_to(any_lexem));
    assert_that(lb.next, is_equal_to(&lb.lexemStream)); /* Should not have moved */
}

Ensure(LexemBuffer, can_put_and_get_lines) {
    char *pointer_after_put = NULL;

    putLexLines(&lb, 13);
    pointer_after_put = lb.end;

    assert_that(getLexToken(&lb.next), is_equal_to(LINE_TOKEN));
    assert_that(getLexToken(&lb.next), is_equal_to(13));
    assert_that(lb.next, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_put_and_get_position) {
    char    *pointer_after_put = NULL;
    Position first_position  = {41, 42, 43};
    Position second_position  = {44, 45, 46};
    Position read_position;

    putLexPositionFields(&lb, first_position.file, first_position.line, first_position.col);
    putLexPosition(&lb, second_position);
    pointer_after_put = lb.end;

    read_position = getLexPosition(&lb.next);
    assert_that(positionsAreEqual(read_position, first_position));
    read_position = getLexPosition(&lb.next);
    assert_that(positionsAreEqual(read_position, second_position));
    assert_that(lb.next, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_backpatch_lexem) {
    Lexem backpatched_lexem = STRING_LITERAL;

    void *backpatchPointer = getLexemStreamEnd(&lb);
    putLexChar(&lb, 'x');

    char *lb_end = lb.end;
    char *lb_next = lb.next;

    putLexTokenAtPointer(backpatched_lexem, backpatchPointer);

    assert_that(getLexemAt(&lb, backpatchPointer), is_equal_to(backpatched_lexem));

    /* lb pointers should not have moved */
    assert_that(lb_end, is_equal_to(lb.end));
    assert_that(lb_next, is_equal_to(lb.next));
}

Ensure(LexemBuffer, can_set_next_write_position) {
    putLexToken(&lb, IDENTIFIER);
    assert_that(lb.end, is_not_equal_to(lb.lexemStream));

    setLexemStreamEnd(&lb, &lb.lexemStream);
    assert_that(lb.end, is_equal_to(lb.lexemStream));
}

Ensure(LexemBuffer, can_get_current_write_position) {
    void *previous = getLexemStreamEnd(&lb);

    putLexChar(&lb, 'x');

    assert_that(getLexemStreamEnd(&lb), is_equal_to(previous+1));
}

Ensure(LexemBuffer, can_write_lexem_at_position_without_changing_pointer) {
    void *writePointer = getLexemStreamEnd(&lb);
    void *savedWritePointer = writePointer;

    putLexTokenAtPointer(IDENTIFIER, writePointer);

    assert_that(writePointer, is_equal_to(savedWritePointer));
    assert_that(getLexToken(&(lb.next)), is_equal_to(IDENTIFIER));
}

Ensure(LexemBuffer, can_write_position_with_pointer) {
    Position position = { .file = 1, .line = 2, .col = 3 };

    putLexPositionWithPointer(position, &(lb.end));

    Position expected_position = getLexPosition(&(lb.next));

    assert_that(positionsAreEqual(position, expected_position));
}
