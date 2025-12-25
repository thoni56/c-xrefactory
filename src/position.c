#include "position.h"
#include "stackmemory.h"

Position noPosition = {-1, 0, 0};

Position makePosition(int file, int line, int col) {
    return (Position){.file = file, .line = line, .col = col};
}

PositionList *newPositionList(Position position, PositionList *next) {
    PositionList *new = stackMemoryAlloc(sizeof(PositionList));
    new->position = position;
    new->next = next;
    return new;
}

bool onSameLine(Position pos1, Position pos2) {
    return pos1.file == pos2.file && pos1.line == pos2.line;
}

Position addPositions(Position p1, Position p2) {
    return makePosition(p1.file+p2.file, p1.line+p2.line, p1.col+p2.col);
}

Position subtractPositions(Position minuend, Position subtrahend) {
    return makePosition(minuend.file-subtrahend.file, minuend.line-subtrahend.line, minuend.col-subtrahend.col);
}

bool positionsAreEqual(Position p1, Position p2) {
    return p1.file == p2.file
        && p1.line == p2.line
        && p1.col == p2.col;
}

bool positionsAreNotEqual(Position p1, Position p2) {
    return !positionsAreEqual(p1, p2);
}

bool positionIsLessThan(Position p1, Position p2) {
    return p1.file < p2.file
        || ((p1.file==p2.file && (p1.line < p2.line
                                  || (p1.line==p2.line && p1.col < p2.col))));
}

bool positionIsLessOrEqualTo(Position p1, Position p2) {
    return positionIsLessThan(p1, p2) || positionsAreEqual(p1, p2);
}

bool positionIsBetween(Position thePosition, Position before, Position after) {
    return before.file == thePosition.file && thePosition.file == after.file &&
        positionIsLessOrEqualTo(before, thePosition) &&
        positionIsLessOrEqualTo(thePosition, after);
}
