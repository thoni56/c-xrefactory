#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

#include "lexembuffer.h"


extern bool getLexemFromLexer(LexemBuffer *lb);
extern void gotOnLineCxRefs(Position *position);

#endif
