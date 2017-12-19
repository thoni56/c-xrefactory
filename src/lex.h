#ifndef LEX_H
#define LEX_H

#include "proto.h"

extern void charBuffClose(struct charBuf *bb);
extern int getCharBuf(struct charBuf *bb);
extern void switchToZippedCharBuff(struct charBuf *bb);
extern int skipNCharsInCharBuf(struct charBuf *bb, unsigned count);
extern int getLexBuf(struct lexBuf *lb);
extern void gotOnLineCxRefs( S_position *ps );

#endif
