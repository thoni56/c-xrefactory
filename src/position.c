#include "position.h"

void fillPosition(Position *position, int file, int line, int col) {
    position->file = file;
    position->line = line;
    position->col = col;
}

void fillPositionList(PositionList *positionList, Position p, PositionList *next) {
    positionList->p = p;
    positionList->next = next;
}
