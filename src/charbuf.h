#ifndef _CHARBUF_H_
#define _CHARBUF_H_

#include "proto.h"

extern void charBuffClose(struct CharacterBuffer *buffer);
extern voidpf zlibAlloc(voidpf opaque, uInt items, uInt size);
extern void zlibFree(voidpf opaque, voidpf address);
extern int getCharBuf(struct CharacterBuffer *buffer);
extern void switchToZippedCharBuff(struct CharacterBuffer *buffer);
extern int skipNCharsInCharBuf(struct CharacterBuffer *buffer, unsigned count);
extern void gotOnLineCxRefs( S_position *ps );

#endif
