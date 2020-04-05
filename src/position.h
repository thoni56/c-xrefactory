#ifndef _POSITION_H_
#define _POSITION_H_

typedef struct position {
    int file;
    int line;
    int col;
} S_position;

typedef struct positionList {
    struct position     p;
    struct positionList *next;
} S_positionList;

extern void fillPosition(S_position *position, int file, int line, int col);
extern void fillPositionList(S_positionList *positionList, S_position p, S_positionList *next);
#endif
