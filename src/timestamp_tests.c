#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "timestamp.h"


Describe(Timestamp);
BeforeEach(Timestamp) {}
AfterEach(Timestamp) {}

Ensure(Timestamp, equal_timestamps_are_equal) {
    FileTimestamp a = makeFileTimestamp(1000, 0);
    FileTimestamp b = makeFileTimestamp(1000, 0);
    assert_that(fileTimestampsEqual(a, b), is_true);
}

Ensure(Timestamp, different_seconds_are_not_equal) {
    FileTimestamp a = makeFileTimestamp(1000, 0);
    FileTimestamp b = makeFileTimestamp(1001, 0);
    assert_that(fileTimestampsEqual(a, b), is_false);
}

Ensure(Timestamp, different_nanoseconds_are_not_equal) {
    FileTimestamp a = makeFileTimestamp(1000, 0);
    FileTimestamp b = makeFileTimestamp(1000, 1);
    assert_that(fileTimestampsEqual(a, b), is_false);
}

Ensure(Timestamp, now_returns_non_zero) {
    FileTimestamp now = fileTimestampNow();
    assert_that(fileTimestampSeconds(now), is_not_equal_to(0));
}

Ensure(Timestamp, can_get_seconds) {
    FileTimestamp ts = makeFileTimestamp(1000, 0);
    assert_that(fileTimestampSeconds(ts), is_equal_to(1000));
}

Ensure(Timestamp, can_get_nanoseconds) {
    FileTimestamp ts = makeFileTimestamp(1000, 500);
    assert_that(fileTimestampNanoseconds(ts), is_equal_to(500));
}

Ensure(Timestamp, can_create_from_seconds_and_nanoseconds) {
    FileTimestamp ts = makeFileTimestamp(1000, 500);
    assert_that(fileTimestampSeconds(ts), is_equal_to(1000));
    assert_that(fileTimestampNanoseconds(ts), is_equal_to(500));
}

Ensure(Timestamp, zero_timestamp_has_zero_seconds_and_nanoseconds) {
    FileTimestamp ts = ZERO_TIMESTAMP;
    assert_that(fileTimestampSeconds(ts), is_equal_to(0));
    assert_that(fileTimestampNanoseconds(ts), is_equal_to(0));
}

Ensure(Timestamp, zero_timestamp_is_zero) {
    assert_that(fileTimestampIsZero(ZERO_TIMESTAMP), is_true);
}

Ensure(Timestamp, non_zero_timestamp_is_not_zero) {
    assert_that(fileTimestampIsZero(makeFileTimestamp(1000, 0)), is_false);
}

Ensure(Timestamp, earlier_is_less_than_later) {
    FileTimestamp a = makeFileTimestamp(1000, 0);
    FileTimestamp b = makeFileTimestamp(1001, 0);
    assert_that(fileTimestampIsLessThan(a, b), is_true);
    assert_that(fileTimestampIsLessThan(b, a), is_false);
}

Ensure(Timestamp, same_seconds_earlier_nsec_is_less_than) {
    FileTimestamp a = makeFileTimestamp(1000, 100);
    FileTimestamp b = makeFileTimestamp(1000, 200);
    assert_that(fileTimestampIsLessThan(a, b), is_true);
    assert_that(fileTimestampIsLessThan(b, a), is_false);
}

Ensure(Timestamp, equal_is_not_less_than) {
    FileTimestamp a = makeFileTimestamp(1000, 100);
    FileTimestamp b = makeFileTimestamp(1000, 100);
    assert_that(fileTimestampIsLessThan(a, b), is_false);
}
