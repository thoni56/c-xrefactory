#ifndef LEX_H
#define LEX_H

#include "proto.h"

extern void charBuffClose(struct CharacterBuffer *bb);
extern int getCharBuf(struct CharacterBuffer *bb);
extern void switchToZippedCharBuff(struct CharacterBuffer *bb);
extern int skipNCharsInCharBuf(struct CharacterBuffer *bb, unsigned count);
extern int getLexBuf(struct lexBuf *lb);
extern void gotOnLineCxRefs( S_position *ps );

#endif
