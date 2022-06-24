#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "characterreader.h"

#include "commons.mock"
#include "fileio.mock"



Describe(CharacterReader);
BeforeEach(CharacterReader) {}
AfterEach(CharacterReader) {}


Ensure(CharacterReader, can_get_string) {
    CharacterBuffer cb;
    char read_string[100];

    initCharacterBufferFromString(&cb, "the string");

    getString(read_string, strlen("the string"), &cb);

    assert_that(read_string, is_equal_to_string("the string"));
}
