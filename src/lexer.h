#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <stdbool.h>

#include "lexembuffer.h"


extern bool getLexemFromLexer(CharacterBuffer *cb, LexemBuffer *lb);
extern void gotOnLineCxRefs(Position *position);

#endif
