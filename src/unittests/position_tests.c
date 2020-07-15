#include <cgreen/cgreen.h>

#include "position.h"

Describe(CxRef);
BeforeEach(CxRef) {}
AfterEach(CxRef) {}


Ensure(CxRef, can_add_two_positions) {
    Position pos = {0, 0, 0};
    Position pos1 = {1, 1, 1};
    Position pos2 = {2, 2, 2};

    addPositionsInto(&pos, pos1, pos2);
    assert_that(pos.file, is_equal_to(3));
    assert_that(pos.line, is_equal_to(3));
    assert_that(pos.col, is_equal_to(3));
}


Ensure(CxRef, can_subtract_two_positions) {
    Position difference = {0, 0, 0};
    Position minuend = {5, 5, 5};
    Position subtrahend = {2, 2, 2};

    subtractPositionsInto(&difference, minuend, subtrahend);
    assert_that(difference.file, is_equal_to(3));
    assert_that(difference.line, is_equal_to(3));
    assert_that(difference.col, is_equal_to(3));
}
