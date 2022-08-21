#ifndef CHARACTERBUFFER_H_INCLUDED
#define CHARACTERBUFFER_H_INCLUDED

#include <zlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "constants.h"


typedef enum {
    INPUT_DIRECT,
    INPUT_VIA_UNZIP,
    INPUT_VIA_EDITOR
} InputMethod;

typedef struct {
    char        *nextUnread;				/* first unread */
    char        *end;				/* pointing after valid characters */
    char        chars[CHAR_BUFF_SIZE];
    FILE        *file;
    unsigned	filePos;			/* how many chars was read from file */
    int			fileNumber;
    int         lineNumber;
    char        *lineBegin;
    int         columnOffset;		/* column == cc-lineBegin + columnOffset */
    bool		isAtEOF;
    InputMethod	inputMethod;		/* unzip/direct */
    char        z[CHAR_BUFF_SIZE];  /* zip input buffer */
    z_stream	zipStream;
} CharacterBuffer;



extern void initCharacterBuffer(CharacterBuffer *characterBuffer, FILE *file);
extern void initCharacterBufferFromString(CharacterBuffer *characterbuffer, char *string);
extern void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                                char *nextUnread,
                                char *end,
                                FILE *file,
                                unsigned filePos,
                                int fileNumber,
                                char *lineBegin);

extern bool refillBuffer(CharacterBuffer *buffer);
extern void switchToZippedCharBuff(CharacterBuffer *buffer);

extern int columnPosition(CharacterBuffer *cb);
extern int absoluteFilePosition(CharacterBuffer *cb);

extern int skipBlanks(CharacterBuffer *cb, int ch);
extern int skipWhiteSpace(CharacterBuffer *cb, int ch);
extern void skipCharacters(CharacterBuffer *buffer, unsigned count);

extern int getChar(CharacterBuffer *cb);
extern void ungetChar(CharacterBuffer *cb, int ch);
extern void getString(char *string, int length, CharacterBuffer *cb);

extern void closeCharacterBuffer(CharacterBuffer *buffer);

#endif
