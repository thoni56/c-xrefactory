#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "characterreader.h"

#include "commons.mock"
#include "fileio.mock"
#include "log.h"

Describe(CharacterReader);
BeforeEach(CharacterReader) {
    log_set_level(LOG_ERROR);
}
AfterEach(CharacterReader) {}

static CharacterBuffer cb;

Ensure(CharacterReader, can_get_string) {
    char            read_string[100];
    char            expected_string[] = "the string";

    initCharacterBufferFromString(&cb, expected_string);

    getString(&cb, read_string, strlen(expected_string));

    assert_that(read_string, is_equal_to_string(expected_string));
}

Ensure(CharacterReader, can_read_and_unread) {
    initCharacterBufferFromString(&cb, "some string");

    int ch = getChar(&cb);

    assert_that(ch, is_equal_to('s'));
    assert_that(cb.nextUnread, is_equal_to(cb.chars + 1));

    ungetChar(&cb, ch);
    assert_that(cb.nextUnread, is_equal_to(cb.chars));
}

Ensure(CharacterReader, can_skip_whitespace) {
    char string_starting_with_whitespace[] = " \t \nnomorewhitespace";

    initCharacterBufferFromString(&cb, string_starting_with_whitespace);

    int ch = skipWhiteSpace(&cb, ' ');

    assert_that(ch, is_equal_to('n'));
    assert_that(cb.nextUnread, is_equal_to(&cb.chars[5]));
}
