#ifndef _CHARACTERBUFFER_H_
#define _CHARACTERBUFFER_H_

#include <zlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "constants.h"


typedef enum {
    INPUT_DIRECT,
    INPUT_VIA_UNZIP,
    INPUT_VIA_EDITOR
} InputMethod;

typedef struct characterBuffer {
    char        *next;				/* first unread */
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
extern void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                                char *next,
                                char *end,
                                FILE *file,
                                unsigned filePos,
                                int fileNumber,
                                char *lineBegin);
extern void closeCharacterBuffer(CharacterBuffer *buffer);
extern voidpf zlibAlloc(voidpf opaque, uInt items, uInt size);
extern void zlibFree(voidpf opaque, voidpf address);
extern bool refillBuffer(CharacterBuffer *buffer);
extern void switchToZippedCharBuff(CharacterBuffer *buffer);
extern int skipNCharsInCharBuf(CharacterBuffer *buffer, unsigned count);

#endif
