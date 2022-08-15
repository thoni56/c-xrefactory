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
    char *end;				/* pointing *after* last valid lexem */
    char lexemStream[LEXEM_BUFFER_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int ringIndex;           /* ...Ring[ringIndex%LEX_POSITIONS_RING_SIZE] */
    CharacterBuffer buffer;
} LexemBuffer;


extern void setLexemStreamEnd(LexemBuffer *lb, void *end);
extern void *getLexemStreamEnd(LexemBuffer *lb);

/* Lexer functions for passing compressed tokens to the parser */
extern int fileNumberFrom(LexemBuffer *lb);
extern int lineNumberFrom(LexemBuffer *lb);

extern void putLexInt(LexemBuffer *lb, int value);
extern void putLexChar(LexemBuffer *lb, char ch);
extern void putLexLines(LexemBuffer *lb, int lines);

extern void putLexToken(LexemBuffer *lb, Lexem lexem);
extern void putLexTokenAtPointer(Lexem lexem, void *writePointer);

extern void putLexPositionFields(LexemBuffer *lb, int file, int line, int col);
extern void putLexPosition(LexemBuffer *lb, Position position);

/* DEPRECATED - Writes at where writePointer points to and advances it */
extern void putLexTokenWithPointer(Lexem lexem, char **writePointerP);
extern void putLexPositionWithPointer(Position position, char **writePointerP);
extern void putLexIntWithPointer(int integer, char **writePointerP);


extern Lexem getLexemAt(LexemBuffer *lb, void *readPointer);

extern unsigned char getLexChar(char **readPointer);
extern Lexem getLexToken(char **readPointer);
extern int getLexInt(char **readPointer);

extern Lexem peekLexToken(char **readPointer);

extern Position getLexPosition(char **readPointer);

/* TODO: cannot replace NextLexPosition macro with this yet, as the only call has "bcc+1" as tmpcc */
extern Position peekLexPosition(char **readPointer);

#define NextLexPosition(pos,tmpcc) {            \
        char *tmptmpcc = tmpcc;                 \
        pos = getLexPosition(&tmptmpcc);        \
    }

extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);

#endif
