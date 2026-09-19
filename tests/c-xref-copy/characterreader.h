#ifndef CHARACTERBUFFER_H_INCLUDED
#define CHARACTERBUFFER_H_INCLUDED

#include <stdio.h>
#include <stdbool.h>
#include "constants.h"


typedef struct {
    char       *nextUnread; /* first unread */
    char       *end;        /* pointing after valid characters */
    char        chars[CHARACTER_BUFFER_SIZE];
    FILE       *file;
    unsigned    filePos; /* how many chars was read from file */
    int         fileNumber;
    int         lineNumber;
    char       *lineBegin;
    int         columnOffset; /* column == cc-lineBegin + columnOffset */
    bool        isAtEOF;
} CharacterBuffer;

extern void initCharacterBufferFromFile(CharacterBuffer *characterBuffer, FILE *file);
extern void initCharacterBufferFromString(CharacterBuffer *characterbuffer, char *string);
extern void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                                char *nextUnread,
                                char *end,
                                FILE *file,
                                unsigned filePos,
                                int fileNumber,
                                char *lineBegin);

extern bool refillBuffer(CharacterBuffer *buffer);

/* Lexer functions for passing compressed tokens to the parser */
extern int fileNumberFrom(CharacterBuffer *cb);
extern int lineNumberFrom(CharacterBuffer *cb);

extern int columnPosition(CharacterBuffer *cb);
extern int fileOffsetFor(CharacterBuffer *cb);

extern int skipBlanks(CharacterBuffer *cb, int ch);
extern int skipWhiteSpace(CharacterBuffer *cb, int ch);
extern void skipCharacters(CharacterBuffer *buffer, unsigned count);

extern int getChar(CharacterBuffer *cb);
extern void ungetChar(CharacterBuffer *cb, int ch);
extern void getString(CharacterBuffer *cb, char *string, int length);

extern void closeCharacterBuffer(CharacterBuffer *buffer);

#endif
