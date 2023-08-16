#ifndef LEXEMBUFFER_H_INCLUDED
#define LEXEMBUFFER_H_INCLUDED

#include "lexem.h"
#include "characterreader.h"
#include "position.h"


/* Sizes in bytes/chars of the lexem code for tokens and identifiers */
#define LEXEMCODE_SIZE 2


typedef struct {
    char *begin;                /* where it begins */
    char *read;                 /* next to read */
    char *write;				/* where to write next */
    char lexemStream[LEXEM_BUFFER_SIZE];
    Position positionRing[LEX_POSITIONS_RING_SIZE];
    unsigned fileOffsetRing[LEX_POSITIONS_RING_SIZE];
    int ringIndex;           /* ...Ring[ringIndex%LEX_POSITIONS_RING_SIZE] */
    char *backpatchPointer; /* Pointer into lexemStream can be saved
                              * and later backpatched without explicit
                              * knowledge of buffers and indices... */
} LexemBuffer;


/* LexemBuffer manipulation */
extern void shiftAnyRemainingLexems(LexemBuffer *lb);

extern void *getLexemStreamWrite(LexemBuffer *lb);

extern void backpatchLexemCodeAt(LexemCode lexem, void *writePointer);

extern void saveBackpatchPosition(LexemBuffer *lb);
extern void backpatchLexemCode(LexemBuffer *lb, LexemCode lexem);

extern void moveLexemStreamWriteToBackpatchPositonWithOffset(LexemBuffer *lb, int offset);

extern int strlenOfBackpatchedIdentifier(LexemBuffer *lb);

    /* Put elementary values */
extern void putLexemInt(LexemBuffer *lb, int value);
extern void putLexemChar(LexemBuffer *lb, char ch);
extern void putLexemCode(LexemBuffer *lb, LexemCode lexem);

/* Put semantically meaningful complete lexems including position, string, value, ... */
extern void putLexemLines(LexemBuffer *lb, int lines);
extern int putIdentifierLexem(LexemBuffer *lexemBuffer, CharacterBuffer *characterBuffer, int ch);
extern void putLexemPosition(LexemBuffer *lb, Position position);
extern void putLexemPositionFields(LexemBuffer *lb, int file, int line, int col);
extern void putLexemWithColumn(LexemBuffer *lb, LexemCode lexem, CharacterBuffer *cb, int column);
extern int  putIncludeString(LexemBuffer *lb, CharacterBuffer *cb, int ch);
extern void putIntegerLexem(LexemBuffer *lb, LexemCode lexem, long unsigned value, CharacterBuffer *cb,
                            int lexemStartingColumn, int lexStartFilePos);
extern void putCompletionLexem(LexemBuffer *lb, CharacterBuffer *cb, int len);
extern void putFloatingPointLexem(LexemBuffer *lb, LexemCode lexem, CharacterBuffer *cb,
                                  int lexemStartingColumn, int lexStartFilePos);

/* DEPRECATED? - Writes at where writePointer points to and advances it */
extern void putLexemCodeAt(LexemCode lexem, char **writePointerP);
extern void putLexemPositionAt(Position position, char **writePointerP);
extern void putLexemIntAt(int integer, char **writePointerP);

/* Get semantically meaningful lexems */
extern Position getLexemPosition(LexemBuffer *lb);


/* TODO: These should be replaced by functions only taking a LexemBuffer... */

/* Reads where a readPointer points and advances it */
extern LexemCode getLexemCodeAt(char **readPointerP);
extern int getLexemIntAt(char **readPointerP);
extern Position getLexemPositionAt(char **readPointerP);

extern LexemCode peekLexemCodeAt(char *readPointer);
extern Position peekLexemPositionAt(char *readPointer);

extern void initLexemBuffer(LexemBuffer *buffer);

#endif
