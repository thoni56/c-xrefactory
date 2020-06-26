#ifndef _LEXEMBUFFER_H_
#define _LEXEMBUFFER_H_

#include "lexem.h"
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


/* Lexer macros for passing compressed tokens to the parser */
/* In the process of being turned into testable/debuggable functions */

extern void putLexChar(char ch, char **writePointer);
extern void putLexShort(int shortValue, char **writePointer);
extern void putLexToken(Lexem lexem, char **writePointer);
extern void putLexInt(int value, char **writePointer);
extern void putLexCompacted(int value, char **writePointer);
extern void putLexLines(int lines, char **writePointer);

extern unsigned char getLexChar(char **readPointer);
extern int getLexShort(char **readPointer);
extern Lexem getLexToken(char **readPointer);
extern int getLexInt(char **readPointer);
extern int getLexCompacted(char **readPointer);

extern Lexem nextLexToken(char **readPointer);

/* NORMAL compacted tokens, HUGE mode also existed originally */
/* Can only store file, line, column < 22 bits */

extern void putLexPosition(int file, int line, int col, char **writePointer);
extern Position getLexPosition(char **readPointer);


/* TODO: cannot replace NextLexPosition yet, as the only call has "bcc+1" as dd */
extern Position nextLexPosition(char **readPointer);

#define NextLexPosition(pos,tmpcc) {            \
        char *tmptmpcc = tmpcc;                 \
        pos = getLexPosition(&tmptmpcc);        \
    }

extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);

#endif
