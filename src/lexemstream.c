#include "lexemstream.h"
#include "lexem.h"
#include "lexembuffer.h"
#include <string.h>

LexemStream currentInput;

LexemStream makeLexemStream(char *begin, char *read, char *write, char *macroName, LexemStreamType streamType) {
    LexemStream stream;
    stream.begin      = begin;
    stream.read       = read;
    stream.write      = write;
    stream.macroName  = macroName;
    stream.streamType = streamType;

    return stream;
}

bool lexemStreamHasMore(LexemStream *stream) {
    return stream->read < stream->write;
}

/* Skips over extra lexem information (string, position, value, etc.) without
 * extracting it. This is used when we only want to advance past a lexem. */
void skipExtraLexemInformationFor(LexemCode lexem, char **readPointerP) {
    if (lexem > MULTI_TOKENS_START) {
        if (lexem == IDENTIFIER || lexem == IDENT_NO_CPP_EXPAND || lexem == IDENT_TO_COMPLETE ||
            lexem == STRING_LITERAL) {
            /* Skip string and position */
            *readPointerP = strchr(*readPointerP, '\0') + 1;
            getLexemPositionAt(readPointerP);  /* Advances pointer */
        } else if (lexem == LINE_TOKEN) {
            getLexemIntAt(readPointerP);
        } else if (lexem == CONSTANT || lexem == LONG_CONSTANT) {
            getLexemIntAt(readPointerP);       /* value */
            getLexemPositionAt(readPointerP);  /* position */
            getLexemIntAt(readPointerP);       /* length */
        } else if (lexem == DOUBLE_CONSTANT || lexem == FLOAT_CONSTANT) {
            getLexemPositionAt(readPointerP);  /* position */
            getLexemIntAt(readPointerP);       /* length */
        } else if (lexem == CPP_MACRO_ARGUMENT) {
            getLexemIntAt(readPointerP);       /* argument index */
            getLexemPositionAt(readPointerP);  /* position */
        } else if (lexem == CHAR_LITERAL) {
            getLexemIntAt(readPointerP);       /* value */
            getLexemPositionAt(readPointerP);  /* position */
            getLexemIntAt(readPointerP);       /* length */
        }
    } else if (lexem > CPP_TOKENS_START && lexem < CPP_TOKENS_END) {
        /* Preprocessor tokens have position */
        getLexemPositionAt(readPointerP);
    } else {
        /* Simple tokens might have position */
        getLexemPositionAt(readPointerP);
    }
}

void copyNextLexemFromStreamToStream(LexemStream *inputStream, LexemStream *outputStream) {
    if (!lexemStreamHasMore(inputStream))
        return;

    char *lexemStart = inputStream->read;
    LexemCode lexem = getLexemCodeAndAdvance(&inputStream->read);
    skipExtraLexemInformationFor(lexem, &inputStream->read);

    int lexemLength = inputStream->read - lexemStart;
    memcpy(outputStream->write, lexemStart, lexemLength);
    outputStream->write += lexemLength;
}
