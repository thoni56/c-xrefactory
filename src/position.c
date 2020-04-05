#include "position.h"

void fillPosition(S_position *position, int file, int line, int col) {
    position->file = file;
    position->line = line;
    position->col = col;
}

void fillPositionList(S_positionList *positionList, S_position p, S_positionList *next);
