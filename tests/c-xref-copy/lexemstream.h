#ifndef LEXEMSTREAM_H_INCLUDED
#define LEXEMSTREAM_H_INCLUDED

#include <stdbool.h>
#include "lexem.h"

/* Stream type discriminates buffer ownership and cleanup strategy in refillInputIfEmpty():
 * - NORMAL_STREAM: Buffer from file's lexemBuffer (not owned, no cleanup needed)
 * - MACRO_STREAM: Buffer from macroBodyMemory (owned, must mbmFreeUntil on pop)
 * - MACRO_ARGUMENT_STREAM: Buffer from ppmMemory (signals exception, cleanup elsewhere)
 *
 * Historical note: Previously included CACHED_STREAM for the caching mechanism.
 * The pattern: stream type = buffer source + refill/cleanup strategy.
 */
typedef enum lexemStreamType {
    NORMAL_STREAM,              /* File or local buffer - no arena cleanup */
    MACRO_STREAM,               /* Macro expansion - free from macroBodyMemory */
    MACRO_ARGUMENT_STREAM,      /* Macro argument - signals END_OF_MACRO_ARGUMENT_EXCEPTION */
} LexemStreamType;

typedef struct {
    char *begin;                /* Start of lexem buffer */
    char *read;                 /* Next lexem to read */
    char *write;                /* End of valid lexems */
    char *macroName;            /* For MACRO_STREAM: macro being expanded, else NULL */
    LexemStreamType streamType; /* Discriminates buffer ownership and cleanup strategy */
} LexemStream;


extern LexemStream currentInput;

extern LexemStream makeLexemStream(char *begin, char *read, char *write, char *macroName, LexemStreamType inputType);

/* LexemStream API functions */
bool lexemStreamHasMore(LexemStream *input);
void skipExtraLexemInformationFor(LexemCode lexem, char **readPointerP);
void copyNextLexemFromStreamToStream(LexemStream *inputStream, LexemStream *outputStream);

#endif
