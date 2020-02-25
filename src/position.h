#ifndef _POS_H_
#define _POS_H_

typedef struct position {
    int file;
    int line;
    int col;
} S_position;

typedef struct positionList {
    struct position     p;
    struct positionList *next;
} S_positionList;

#endif
