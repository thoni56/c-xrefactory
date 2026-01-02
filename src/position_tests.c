#include <cgreen/cgreen.h>

#include "position.h"

#include "commons.mock"
#include "stackmemory.mock"


Describe(Position);
BeforeEach(Position) {}
AfterEach(Position) {}


Ensure(Position, can_add_two_positions) {
    Position pos  = {0, 0, 0};
    Position pos1 = {1, 1, 1};
    Position pos2 = {2, 2, 2};

    pos = addPositions(pos1, pos2);
    assert_that(pos.file, is_equal_to(3));
    assert_that(pos.line, is_equal_to(3));
    assert_that(pos.col, is_equal_to(3));
}

Ensure(Position, can_subtract_two_positions) {
    Position difference = {0, 0, 0};
    Position minuend    = {5, 5, 5};
    Position subtrahend = {2, 2, 2};

    difference = subtractPositions(minuend, subtrahend);
    assert_that(difference.file, is_equal_to(3));
    assert_that(difference.line, is_equal_to(3));
    assert_that(difference.col, is_equal_to(3));
}

Ensure(Position, can_see_if_positions_are_equal) {
    Position p1 = (Position){.file = 12, .line = 13, .col = 14};
    Position p2 = (Position){.file = 12, .line = 13, .col = 14};

    assert_that(positionsAreEqual(p1, p2));

    p2.col++;
    assert_that(!positionsAreEqual(p1, p2));
    p2.col--;

    p2.line++;
    assert_that(!positionsAreEqual(p1, p2));
    p2.line--;

    p2.file++;
    assert_that(!positionsAreEqual(p1, p2));
    assert_that(positionsAreNotEqual(p1, p2));
}

Ensure(Position, can_see_if_position_is_less_than) {
    Position p1 = (Position){.file = 12, .line = 13, .col = 14};
    Position p2 = (Position){.file = 12, .line = 13, .col = 14};

    assert_that(!positionIsLessThan(p1, p2));

    p1.col--;
    assert_that(positionIsLessThan(p1, p2));
    p1.col++;

    p1.line--;
    assert_that(positionIsLessThan(p1, p2));
    p1.line++;

    p1.file--;
    assert_that(positionIsLessThan(p1, p2));
}

Ensure(Position, can_see_if_position_is_less_or_equal_to) {
    Position p1 = (Position){.file = 12, .line = 13, .col = 14};
    Position p2 = (Position){.file = 12, .line = 13, .col = 14};

    assert_that(positionIsLessOrEqualTo(p1, p2));

    p1.col--;
    assert_that(positionIsLessOrEqualTo(p1, p2));
    p1.col++;

    p1.line--;
    assert_that(positionIsLessOrEqualTo(p1, p2));
    p1.line++;

    p1.file--;
    assert_that(positionIsLessOrEqualTo(p1, p2));
}

Ensure(Position, can_see_if_position_is_between) {
    Position p1 = (Position){.file = 12, .line = 13, .col = 14};
    Position p  = (Position){.file = 12, .line = 13, .col = 14};
    Position p2 = (Position){.file = 12, .line = 13, .col = 14};

    assert_that(positionIsBetween(p, p1, p2));
}
