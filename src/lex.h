#ifndef LEX_H
#define LEX_H

#include <stdbool.h>

#include "proto.h"
#include "characterbuffer.h"


typedef struct lexBuf {
    char            *next;				/* next to read */
    char            *end;				/* pointing *after* last valid char */
    char            chars[LEX_BUFF_SIZE];
    struct position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned        fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int             index;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    struct characterBuffer buffer;
} S_lexBuf;


extern bool getLexBuf(S_lexBuf *lb);
extern void gotOnLineCxRefs(Position *position);

#endif
