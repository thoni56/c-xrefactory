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

bool onSameLine(Position pos1, Position pos2) {
    return pos1.file == pos2.file && pos1.line == pos2.line;
}
