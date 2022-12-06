#ifndef LEXEMBUFFER_H_INCLUDED
#define LEXEMBUFFER_H_INCLUDED

#include "lexem.h"
#include "characterreader.h"
#include "position.h"


/* Sizes in bytes/chars of tokens and identifiers */
#define TOKEN_SIZE 2
#define IDENT_TOKEN_SIZE 2


typedef struct {
    char *read;                 /* next to read */
    char *begin;
    char *write;				/* where to write lexem */
    char lexemStream[LEXEM_BUFFER_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int ringIndex;           /* ...Ring[ringIndex%LEX_POSITIONS_RING_SIZE] */
} LexemBuffer;


extern void shiftAnyRemainingLexems(LexemBuffer *lb);

extern void setLexemStreamWrite(LexemBuffer *lb, void *end);
extern void *getLexemStreamWrite(LexemBuffer *lb);

/* WRITE */
extern void putLexInt(LexemBuffer *lb, int value);
extern void putLexChar(LexemBuffer *lb, char ch);
extern void putLexLines(LexemBuffer *lb, int lines);

extern void putLexToken(LexemBuffer *lb, Lexem lexem);
extern void putLexTokenAtPointer(Lexem lexem, void *writePointer);

extern void putLexPositionFields(LexemBuffer *lb, int file, int line, int col);
extern void putLexPosition(LexemBuffer *lb, Position position);

/* DEPRECATED? - Writes at where writePointer points to and advances it */
extern void putLexTokenAt(Lexem lexem, char **writePointerP);
extern void putLexPositionAt(Position position, char **writePointerP);
extern void putLexIntAt(int integer, char **writePointerP);

/* READ */
extern Position getLexPosition(LexemBuffer *lb);

/* DEPRECATED? - Reads where a readPointer points and advances it */
extern Lexem getLexemAt(LexemBuffer *lb, void *readPointer); /* TODO - remove readPointeP */
extern Lexem getLexTokenAt(char **readPointerP);
extern int getLexIntAt(char **readPointerP);
extern int getLexShortAt(char **readPointer);
extern Position getLexPositionAt(char **readPointerP);

extern Lexem peekLexTokenAt(char *readPointer);

extern Position peekLexPositionAt(char *readPointer);

extern void initLexemBuffer(LexemBuffer *buffer);

#endif
