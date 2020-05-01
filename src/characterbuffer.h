#ifndef _CHARACTERBUFFER_H_
#define _CHARACTERBUFFER_H_

#include <zlib.h>
#include "proto.h"


extern void fillCharacterBuffer(CharacterBuffer *characterBuffer,
                                char *next,
                                char *end,
                                FILE *file,
                                unsigned filePos,
                                int fileNumber,
                                char *lineBegin);
extern void closeCharacterBuffer(struct characterBuffer *buffer);
extern voidpf zlibAlloc(voidpf opaque, uInt items, uInt size);
extern void zlibFree(voidpf opaque, voidpf address);
extern bool fillBuffer(struct characterBuffer *buffer);
extern void switchToZippedCharBuff(struct characterBuffer *buffer);
extern int skipNCharsInCharBuf(struct characterBuffer *buffer, unsigned count);

#endif
