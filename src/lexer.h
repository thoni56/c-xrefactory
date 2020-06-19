#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

#include "lexembuffer.h"


extern void initLexemBuffer(LexemBuffer *buffer, FILE *file);
extern bool getLexem(LexemBuffer *lb);
extern void gotOnLineCxRefs(Position *position);

#endif
