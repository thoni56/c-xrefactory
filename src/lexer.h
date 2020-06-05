#ifndef _LEX_H_
#define _LEX_H_

#include <stdbool.h>

#include "proto.h"
#include "characterreader.h"


typedef struct lexemBuffer {
    char            *next;				/* next to read */
    char            *end;				/* pointing *after* last valid char */
    char            chars[LEX_BUFF_SIZE];
    struct position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned        fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int             index;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    CharacterBuffer buffer;
} LexemBuffer;


extern bool getLexem(LexemBuffer *lb);
extern void gotOnLineCxRefs(Position *position);

#endif
