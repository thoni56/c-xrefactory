#ifndef LEXEMSTREAM_H_INCLUDED
#define LEXEMSTREAM_H_INCLUDED

#include <stdbool.h>
#include "lexem.h"

typedef enum lexemStreamType {
    NORMAL_STREAM,
    MACRO_STREAM,
    MACRO_ARGUMENT_STREAM,
    INPUT_CACHE,                /* TODO: Remove this value, not used */
} LexemStreamType;

typedef struct {
    char *begin;                /* where it begins */
    char *read;                 /* next to read */
    char *write;                /* where to write next */
    char *macroName;
    LexemStreamType streamType;
} LexemStream;


extern LexemStream currentInput;

extern LexemStream makeLexemStream(char *begin, char *read, char *write, char *macroName, LexemStreamType inputType);

/* LexemStream API functions */
bool lexemStreamHasMore(LexemStream *input);
void skipExtraLexemInformationFor(LexemCode lexem, char **readPointerP);
void copyNextLexemFromStreamToStream(LexemStream *inputStream, LexemStream *outputStream);

#endif
