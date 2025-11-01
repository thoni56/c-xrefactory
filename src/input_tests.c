#include <cgreen/cgreen.h>

#include "input.h"

#include "globals.mock"
#include "options.mock"
#include "commons.mock"
#include "characterreader.mock"


Describe(LexemStream);
BeforeEach(LexemStream) {}
AfterEach(LexemStream) {}

Ensure(LexemStream, lexemStreamHasMore_returns_true_when_read_less_than_write) {
    LexemStream stream = {
        .begin = (char*)"begin",
        .read = (char*)"read_at_pos_5",
        .write = (char*)"write_at_pos_10",
        .macroName = NULL,
        .streamType = NORMAL_STREAM
    };

    assert_true(lexemStreamHasMore(&stream));
}

Ensure(LexemStream, lexemStreamHasMore_returns_false_when_read_equals_write) {
    char buffer[100] = {0};
    LexemStream stream = {
        .begin = buffer,
        .read = buffer + 50,
        .write = buffer + 50,  // Same as read
        .macroName = NULL,
        .streamType = NORMAL_STREAM
    };

    assert_false(lexemStreamHasMore(&stream));
}

Ensure(LexemStream, lexemStreamHasMore_returns_false_when_read_greater_than_write) {
    char buffer[100] = {0};
    LexemStream stream = {
        .begin = buffer,
        .read = buffer + 60,  // Past write
        .write = buffer + 50,
        .macroName = NULL,
        .streamType = NORMAL_STREAM
    };

    assert_false(lexemStreamHasMore(&stream));
}

Ensure(LexemStream, lexemStreamHasMore_with_empty_stream) {
    char buffer[10] = {0};
    LexemStream stream = {
        .begin = buffer,
        .read = buffer,
        .write = buffer,  // No data to read
        .macroName = NULL,
        .streamType = NORMAL_STREAM
    };

    assert_false(lexemStreamHasMore(&stream));
}

Ensure(LexemStream, lexemStreamHasMore_with_single_byte_remaining) {
    char buffer[10] = "x";
    LexemStream stream = {
        .begin = buffer,
        .read = buffer,
        .write = buffer + 1,  // One byte to read
        .macroName = NULL,
        .streamType = NORMAL_STREAM
    };

    assert_true(lexemStreamHasMore(&stream));
}
