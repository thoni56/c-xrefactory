#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "head.h"
#include "lexembuffer.h"
#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "globals.mock"
#include "lexem.mock"
#include "options.mock"


static LexemBuffer lb;

Describe(LexemBuffer);
BeforeEach(LexemBuffer) {
    log_set_level(LOG_ERROR);
    initLexemBuffer(&lb);
}
AfterEach(LexemBuffer) {}


protected extern void putLexemCode(LexemBuffer *lb, LexemCode lexem);


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

Ensure(LexemBuffer, can_put_and_get_lexem_code) {
    putLexemCode(&lb, STRING_LITERAL);
    assert_that(getLexemCode(&lb), is_equal_to(STRING_LITERAL));
}

Ensure(LexemBuffer, can_put_and_get_two_lexem_codes) {
    putLexemCode(&lb, DOUBLE_CONSTANT);
    putLexemCode(&lb, STRING_LITERAL);

    assert_that(getLexemCode(&lb), is_equal_to(DOUBLE_CONSTANT));
    assert_that(getLexemCode(&lb), is_equal_to(STRING_LITERAL));
}

extern void putLexCompactedAt(int value, char **writePointer);
extern int getLexCompactedAt(char **readPointer);
Ensure(LexemBuffer, can_put_and_get_a_compacted_int) {
    int         integer;
    char       *expected_next_after_get;

    /* Test values across the 128 boundry, 1 or 2 "slots" */
    for (int i = 0; i < 150; i++) {
        putLexCompactedAt(i, &lb.write);
        expected_next_after_get = lb.write;

        integer     = getLexCompactedAt(&lb.read);

        assert_that(lb.read, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }

    /* Test values across the 16384 boundry, 2 or 3 "slots" */
    for (int i = 16300; i < 16500; i++) {
        putLexCompactedAt(i, &lb.write);
        expected_next_after_get = lb.write;

        integer     = getLexCompactedAt(&lb.read);

        assert_that(lb.read, is_equal_to(expected_next_after_get));
        assert_that(integer, is_equal_to(i));
    }
}

Ensure(LexemBuffer, can_peek_next_token) {
    LexemCode any_lexem = CHAR_LITERAL;

    putLexemCode(&lb, any_lexem);

    assert_that(peekLexemCodeAt(lb.read), is_equal_to(any_lexem));
}

Ensure(LexemBuffer, can_put_and_get_lines) {
    char *pointer_after_put = NULL;

    putLexemLines(&lb, 13); // A LexemCode and a LexemInt
    pointer_after_put = lb.write;

    assert_that(getLexemCodeAndAdvance(&lb.read), is_equal_to(LINE_TOKEN));
    assert_that(getLexemIntAt(&lb.read), is_equal_to(13));
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

    read_position = getLexemPositionAt(&lb.read);
    assert_that(positionsAreEqual(read_position, first_position));
    read_position = getLexemPositionAt(&lb.read);
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
    read_position = getLexemPositionAt(&lb.read);
    assert_that(positionsAreEqual(read_position, second_position));
    assert_that(lb.read, is_equal_to(pointer_after_put));
}

extern void putLexemChar(LexemBuffer *lb, char ch);
Ensure(LexemBuffer, can_backpatch_lexem) {
    LexemCode backpatched_lexem = STRING_LITERAL;

    void *backpatchPointer = getLexemStreamWrite(&lb);
    putLexemChar(&lb, 'x');

    char *lb_end = lb.write;
    char *lb_next = lb.read;

    backpatchLexemCodeAt(backpatched_lexem, backpatchPointer);

    assert_that(peekLexemCodeAt(backpatchPointer), is_equal_to(backpatched_lexem));

    /* lb pointers should not have moved */
    assert_that(lb_end, is_equal_to(lb.write));
    assert_that(lb_next, is_equal_to(lb.read));
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
    assert_that(peekLexemCodeAt(lb.read), is_equal_to(IDENTIFIER));
}

Ensure(LexemBuffer, can_save_batchpatch_position_and_put_there) {
    saveBackpatchPosition(&lb);
    putLexemCode(&lb, CONSTANT);

    backpatchLexemCode(&lb, IDENTIFIER);

    assert_that(peekLexemCodeAt(&(lb.lexemStream[0])), is_equal_to(IDENTIFIER));
}


Ensure(LexemBuffer, can_write_position_with_pointer) {
    Position position = { .file = 1, .line = 2, .col = 3 };

    putLexemPositionAt(position, &(lb.write));

    Position expected_position = getLexemPositionAt(&(lb.read));

    assert_that(positionsAreEqual(position, expected_position));
}
