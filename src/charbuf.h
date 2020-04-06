#ifndef _CHARBUF_H_
#define _CHARBUF_H_

#include <zlib.h>
#include "proto.h"


extern void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                                 char *next,
                                 char *end,
                                 FILE *file,
                                 unsigned filePos,
                                 int fileNumber,
                                 char *lineBegin);
extern void charBuffClose(struct CharacterBuffer *buffer);
extern voidpf zlibAlloc(voidpf opaque, uInt items, uInt size);
extern void zlibFree(voidpf opaque, voidpf address);
extern bool getCharBuf(struct CharacterBuffer *buffer);
extern void switchToZippedCharBuff(struct CharacterBuffer *buffer);
extern int skipNCharsInCharBuf(struct CharacterBuffer *buffer, unsigned count);

#endif
