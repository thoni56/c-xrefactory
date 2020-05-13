#ifndef _POSITION_H_
#define _POSITION_H_

#include <stdbool.h>


typedef struct position {
    int file;
    int line;
    int col;
} Position;

typedef struct positionList {
    struct position     p;
    struct positionList *next;
} PositionList;


extern void fillPosition(Position *position, int file, int line, int col);
extern void fillPositionList(PositionList *positionList, Position p, PositionList *next);
extern bool onSameLine(Position pos1, Position pos2);

#endif
