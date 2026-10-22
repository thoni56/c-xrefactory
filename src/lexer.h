#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include <stdbool.h>

#include "lexembuffer.h"


extern bool buildLexemFromCharacters(CharacterBuffer *cb, LexemBuffer *lb);

#endif
