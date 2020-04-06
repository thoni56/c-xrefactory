#ifndef _CHARBUF_H_
#define _CHARBUF_H_

#include <zlib.h>
#include "proto.h"


extern void fill_CharacterBuffer(CharacterBuffer *characterBuffer,
                                 char *next,
                                 char *end,
                                 FILE *file,
                                 unsigned filePos,
                                 int fileNumber,
                                 int lineNum,
                                 char *lineBegin,
                                 int columnOffset,
                                 bool isAtEOF,
                                 InputMethod inputMethod,
                                 z_stream zipStream);
extern void charBuffClose(struct CharacterBuffer *buffer);
extern voidpf zlibAlloc(voidpf opaque, uInt items, uInt size);
extern void zlibFree(voidpf opaque, voidpf address);
extern bool getCharBuf(struct CharacterBuffer *buffer);
extern void switchToZippedCharBuff(struct CharacterBuffer *buffer);
extern int skipNCharsInCharBuf(struct CharacterBuffer *buffer, unsigned count);

#endif
