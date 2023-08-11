#ifndef LEXEMBUFFER_H_INCLUDED
#define LEXEMBUFFER_H_INCLUDED

#include "lexem.h"
#include "characterreader.h"
#include "position.h"


/* Sizes in bytes/chars of tokens and identifiers */
#define TOKEN_SIZE 2
#define IDENT_TOKEN_SIZE 2


typedef struct {
    char *begin;                /* where it begins */
    char *read;                 /* next to read */
    char *write;				/* where to write next */
    char lexemStream[LEXEM_BUFFER_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int ringIndex;           /* ...Ring[ringIndex%LEX_POSITIONS_RING_SIZE] */
} LexemBuffer;


/* LexemBuffer manipulation */
extern void shiftAnyRemainingLexems(LexemBuffer *lb);

extern void setLexemStreamWrite(LexemBuffer *lb, void *end);
extern void *getLexemStreamWrite(LexemBuffer *lb);

/* Put elementary values */
extern void putLexemInt(LexemBuffer *lb, int value);
extern void putLexemChar(LexemBuffer *lb, char ch);
extern void putLexemCode(LexemBuffer *lb, LexemCode lexem);

/* Put semantically meaningful complete lexems including position, string, value, ... */
extern void backpatchLexemCodeAt(LexemCode lexem, void *writePointer);
extern void putLexemLines(LexemBuffer *lb, int lines);
extern int putIdentifierLexem(LexemBuffer *lexemBuffer, CharacterBuffer *characterBuffer, int ch);
extern void putLexemPosition(LexemBuffer *lb, Position position);
extern void putLexemPositionFields(LexemBuffer *lb, int file, int line, int col);

/* DEPRECATED? - Writes at where writePointer points to and advances it */
extern void putLexTokenAt(LexemCode lexem, char **writePointerP);
extern void putLexPositionAt(Position position, char **writePointerP);
extern void putLexIntAt(int integer, char **writePointerP);

/* READ */
extern Position getLexemPosition(LexemBuffer *lb);


/* TODO: These should be replaced by functions only taking a LexemBuffer... */

/* Reads where a readPointer points and advances it */
extern LexemCode getLexTokenAt(char **readPointerP);
extern int getLexIntAt(char **readPointerP);
extern Position getLexPositionAt(char **readPointerP);

extern LexemCode peekLexTokenAt(char *readPointer);

extern Position peekLexPositionAt(char *readPointer);

extern void initLexemBuffer(LexemBuffer *buffer);

#endif
