#ifndef LEXEMBUFFER_H_INCLUDED
#define LEXEMBUFFER_H_INCLUDED

#include "lexem.h"
#include "characterreader.h"
#include "position.h"


/* Sizes in bytes/chars of tokens and identifiers */
#define TOKEN_SIZE 2
#define IDENT_TOKEN_SIZE 2


typedef struct lexemBuffer {
    char *next;				/* next to read */
    char *end;				/* pointing *after* last valid char */
    char lexemStream[LEX_BUFF_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int index;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    CharacterBuffer buffer;
} LexemBuffer;


/* Lexer functions for passing compressed tokens to the parser */

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

/* NORMAL compacted tokens, HUGE mode also existed originally. NORMAL
   can only store file, line, column < 22 bits which should be
   sufficient for any reasonable case. */

extern void putLexPosition(int file, int line, int col, char **writePointer);
extern Position getLexPosition(char **readPointer);


/* TODO: cannot replace NextLexPosition yet, as the only call has "bcc+1" as tmpcc */
extern Position nextLexPosition(char **readPointer);

#define NextLexPosition(pos,tmpcc) {            \
        char *tmptmpcc = tmpcc;                 \
        pos = getLexPosition(&tmptmpcc);        \
    }

extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);

#endif
