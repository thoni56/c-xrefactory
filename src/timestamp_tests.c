#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "timestamp.h"


Describe(Timestamp);
BeforeEach(Timestamp) {}
AfterEach(Timestamp) {}

Ensure(Timestamp, equal_timestamps_are_equal) {
    FileTimestamp a = 1000;
    FileTimestamp b = 1000;
    assert_that(fileTimestampsEqual(a, b), is_true);
}

Ensure(Timestamp, different_timestamps_are_not_equal) {
    FileTimestamp a = 1000;
    FileTimestamp b = 1001;
    assert_that(fileTimestampsEqual(a, b), is_false);
}
