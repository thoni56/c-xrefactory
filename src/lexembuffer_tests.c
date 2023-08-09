#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "lexem.h"
#include "lexembuffer.h"
#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "zlib.mock"


static LexemBuffer lb;

Describe(LexemBuffer);
BeforeEach(LexemBuffer) {
    log_set_level(LOG_ERROR);
    initLexemBuffer(&lb);
}
AfterEach(LexemBuffer) {}


Ensure(LexemBuffer, can_shift_remaining_lexems) {
    LexemBuffer lb = { .write = lb.lexemStream, .read = lb.lexemStream };
    char *test_string = "abcdefg";

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.read, is_equal_to(lb.lexemStream));
    assert_that(lb.write, is_equal_to(lb.lexemStream));

    lb.lexemStream[1] = '1';
    lb.read = &lb.lexemStream[1];
    lb.write  = &lb.lexemStream[2];

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.read, is_equal_to(lb.lexemStream));
    assert_that(lb.write, is_equal_to(&lb.lexemStream[1]));
    assert_that(lb.lexemStream[0], is_equal_to('1'));

    strcpy(&lb.lexemStream[2], test_string);
    lb.read = &lb.lexemStream[2];
    lb.write  = &lb.lexemStream[9];

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.read, is_equal_to(lb.lexemStream));
    assert_that(lb.write, is_equal_to(&lb.lexemStream[strlen(test_string)]));
    assert_that(strncmp(lb.lexemStream, test_string, strlen(test_string)), is_equal_to(0));
}

extern void putLexShortAt(int shortValue, char **writePointer);
extern int getLexShortAt(char **readPointer);
Ensure(LexemBuffer, can_put_and_get_a_short) {
    int   shortValue;
    char *expected_next_after_get;

    putLexShortAt(433, &lb.write);
    expected_next_after_get = lb.write;

    shortValue  = getLexShortAt(&lb.read);

    assert_that(lb.read, is_equal_to(expected_next_after_get));
    assert_that(shortValue, is_equal_to(433));
}

Ensure(LexemBuffer, can_put_and_get_a_token) {
    LexemCode       lexem;
    char       *expected_next_after_get;

    putLexemCode(&lb, DOUBLE_CONSTANT);
    expected_next_after_get = lb.write;

    lexem = getLexTokenAt(&lb.read);

    assert_that(lb.read, is_equal_to(expected_next_after_get));
    assert_that(lexem, is_equal_to(DOUBLE_CONSTANT));
}

Ensure(LexemBuffer, can_put_and_get_an_int) {
    int         integer;
    char       *expected_next_after_get;

    putLexemInt(&lb, 34581);
    expected_next_after_get = lb.write;

    integer     = getLexIntAt(&lb.read);

    assert_that(lb.read, is_equal_to(expected_next_after_get));
    assert_that(integer, is_equal_to(34581));
}

extern void putLexCompacted(int value, char **writePointer);
extern int getLexCompacted(char **readPointer);
Ensure(LexemBuffer, can_put_and_get_a_compacted_int) {
    int         integer;
    char       *expected_next_after_get;

    /* Test values across the 128 boundry, 1 or 2 "slots" */
    for (int i = 0; i < 150; i++) {
        putLexCompacted(i, &lb.write);
        expected_next_after_get = lb.write;

        integer     = getLexCompacted(&lb.read);

        assert_that(lb.read, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }

    /* Test values across the 16384 boundry, 2 or 3 "slots" */
    for (int i = 16300; i < 16500; i++) {
        putLexCompacted(i, &lb.write);
        expected_next_after_get = lb.write;

        integer     = getLexCompacted(&lb.read);

        assert_that(lb.read, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }
}

Ensure(LexemBuffer, can_peek_next_token) {
    LexemCode any_lexem = CHAR_LITERAL;

    putLexemCode(&lb, any_lexem);

    assert_that(peekLexTokenAt(lb.read), is_equal_to(any_lexem));
}

Ensure(LexemBuffer, can_put_and_get_lines) {
    char *pointer_after_put = NULL;

    putLexemLines(&lb, 13); // A LexemCode and a LexemInt
    pointer_after_put = lb.write;

    assert_that(getLexTokenAt(&lb.read), is_equal_to(LINE_TOKEN));
    assert_that(getLexIntAt(&lb.read), is_equal_to(13));
    assert_that(lb.read, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_put_and_get_position_with_pointer) {
    char    *pointer_after_put = NULL;
    Position first_position  = {41, 42, 43};
    Position second_position  = {44, 45, 46};
    Position read_position;

    putLexemPositionFields(&lb, first_position.file, first_position.line, first_position.col);
    putLexemPosition(&lb, second_position);
    pointer_after_put = lb.write;

    read_position = getLexPositionAt(&lb.read);
    assert_that(positionsAreEqual(read_position, first_position));
    read_position = getLexPositionAt(&lb.read);
    assert_that(positionsAreEqual(read_position, second_position));
    assert_that(lb.read, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_put_and_get_position) {
    char    *pointer_after_put = NULL;
    Position first_position  = {41, 42, 43};
    Position second_position  = {44, 45, 46};
    Position read_position;

    putLexemPositionFields(&lb, first_position.file, first_position.line, first_position.col);
    putLexemPosition(&lb, second_position);
    pointer_after_put = lb.write;

    read_position = getLexemPosition(&lb);
    assert_that(positionsAreEqual(read_position, first_position));
    read_position = getLexPositionAt(&lb.read);
    assert_that(positionsAreEqual(read_position, second_position));
    assert_that(lb.read, is_equal_to(pointer_after_put));
}

Ensure(LexemBuffer, can_backpatch_lexem) {
    LexemCode backpatched_lexem = STRING_LITERAL;

    void *backpatchPointer = getLexemStreamWrite(&lb);
    putLexemChar(&lb, 'x');

    char *lb_end = lb.write;
    char *lb_next = lb.read;

    backpatchLexemCodeAt(backpatched_lexem, backpatchPointer);

    assert_that(getLexemAt(&lb, backpatchPointer), is_equal_to(backpatched_lexem));

    /* lb pointers should not have moved */
    assert_that(lb_end, is_equal_to(lb.write));
    assert_that(lb_next, is_equal_to(lb.read));
}

Ensure(LexemBuffer, can_set_next_write_position) {
    putLexemCode(&lb, IDENTIFIER);
    assert_that(lb.write, is_not_equal_to(lb.lexemStream));

    setLexemStreamWrite(&lb, &lb.lexemStream);
    assert_that(lb.write, is_equal_to(lb.lexemStream));
}

Ensure(LexemBuffer, can_get_current_write_position) {
    void *previous = getLexemStreamWrite(&lb);

    putLexemChar(&lb, 'x');

    assert_that(getLexemStreamWrite(&lb), is_equal_to(previous+1));
}

Ensure(LexemBuffer, can_write_lexem_at_position_without_changing_pointer) {
    void *writePointer = getLexemStreamWrite(&lb);
    void *savedWritePointer = writePointer;

    backpatchLexemCodeAt(IDENTIFIER, writePointer);

    assert_that(writePointer, is_equal_to(savedWritePointer));
    assert_that(getLexTokenAt(&(lb.read)), is_equal_to(IDENTIFIER));
}

Ensure(LexemBuffer, can_write_position_with_pointer) {
    Position position = { .file = 1, .line = 2, .col = 3 };

    putLexPositionAt(position, &(lb.write));

    Position expected_position = getLexPositionAt(&(lb.read));

    assert_that(positionsAreEqual(position, expected_position));
}

Ensure(LexemBuffer, can_write_int_with_pointer) {
    int integer = 144;

    putLexIntAt(integer, &(lb.write));

    int expected_integer = getLexIntAt(&(lb.read));

    assert_that(expected_integer, is_equal_to(integer));
}
