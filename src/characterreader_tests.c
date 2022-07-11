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

Ensure(CharacterReader, can_get_string) {
    CharacterBuffer cb;
    char            read_string[100];

    initCharacterBufferFromString(&cb, "the string");

    getString(read_string, strlen("the string"), &cb);

    assert_that(read_string, is_equal_to_string("the string"));
}

Ensure(CharacterReader, can_read_and_unread) {
    CharacterBuffer cb;

    initCharacterBufferFromString(&cb, "some string");

    int ch = getChar(&cb);

    assert_that(ch, is_equal_to('s'));
    assert_that(cb.nextUnread, is_equal_to(cb.chars + 1));

    ungetChar(&cb, ch);
    assert_that(cb.nextUnread, is_equal_to(cb.chars));
}
