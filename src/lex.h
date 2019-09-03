#ifndef LEX_H
#define LEX_H

#include "proto.h"

extern int getLexBuf(struct lexBuf *lb);
extern void gotOnLineCxRefs( S_position *ps );

#endif
