#ifndef _LEXEMBUFFER_H_
#define _LEXEMBUFFER_H_

#include "characterreader.h"
#include "position.h"


typedef struct lexemBuffer {
    char *next;				/* next to read */
    char *end;				/* pointing *after* last valid char */
    char chars[LEX_BUFF_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int index;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    CharacterBuffer buffer;
} LexemBuffer;


#endif
