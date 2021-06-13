#include <cgreen/cgreen.h>

/* Including the source since we are testing macros */
#include "lexer.c"

#include "globals.mock"
#include "commons.mock"
#include "caching.mock"
#include "cxref.mock"
#include "utils.mock"
#include "jslsemact.mock"
#include "yylex.mock"
#include "filetable.mock"
#include "filedescriptor.mock"
#include "zlib.mock"
#include "fileio.mock"


Describe(Lex);
BeforeEach(Lex) {}
AfterEach(Lex) {}


Ensure(Lex, will_signal_false_for_empty_lexbuffer) {
    LexemBuffer lexemBuffer;

    s_cache.activeCache = false; /* ?? */

    lexemBuffer.next = NULL;
    lexemBuffer.end = NULL;
    lexemBuffer.index = 0;

    initCharacterBuffer(&lexemBuffer.buffer, NULL);

    assert_that(getLexemFromLexer(&lexemBuffer), is_false);
}

Ensure(Lex, can_scan_a_floating_point_number) {
    LexemBuffer lexemBuffer;
    char *lexemPointer = lexemBuffer.lexemStream;
    CharacterBuffer *charBuffer = &lexemBuffer.buffer;
    char *inputString = "4.3f";

    s_cache.activeCache = false; /* ?? */

    initLexemBuffer(&lexemBuffer, NULL);
    charBuffer->fileNumber = 0;

    initCharacterBufferFromString(&lexemBuffer.buffer, inputString);

    assert_that(getLexemFromLexer(&lexemBuffer), is_true);
    assert_that(getLexToken(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}
