#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <stdbool.h>

extern int NO_FILE_NUMBER;      /* Don't pull in all of `filetable.h` */


#define NO_POSITION (Position){.file = NO_FILE_NUMBER, .line = -1, .col = -1}

typedef struct position {
    int file;
    int line;
    int col;
} Position;

typedef struct positionList {
    struct position     position;
    struct positionList *next;
} PositionList;


extern PositionList *newPositionList(Position position, PositionList *next);

extern Position makePosition(int file, int line, int col);
extern bool onSameLine(Position pos1, Position pos2);

extern Position addPositions(Position p1, Position p2);
extern Position subtractPositions(Position minuend, Position subtrahend);
extern bool positionsAreEqual(Position p1, Position p2);
extern bool positionsAreNotEqual(Position p1, Position p2);
extern bool positionIsLessThan(Position p1, Position p2);
extern bool positionIsLessOrEqualTo(Position p1, Position p2);
extern bool positionIsBetween(Position p, Position p1, Position p2);

#endif
