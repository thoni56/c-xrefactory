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
    LexemBuffer lexBuffer;

    s_cache.activeCache = false; /* ?? */

    lexBuffer.next = NULL;
    lexBuffer.end = NULL;
    lexBuffer.index = 0;

    initCharacterBuffer(&lexBuffer.buffer, NULL);

    assert_that(getLexem(&lexBuffer), is_false);
}

Ensure(Lex, can_scan_a_floating_point_number) {
    LexemBuffer lexBuffer;
    char *lexemPointer = lexBuffer.lexemStream;
    CharacterBuffer *charBuffer = &lexBuffer.buffer;
    char *inputString = "4.3f";

    s_cache.activeCache = false; /* ?? */

    initLexemBuffer(&lexBuffer, NULL);
    charBuffer->fileNumber = 0;

    initCharacterBufferFromString(&lexBuffer.buffer, inputString);

    assert_that(getLexem(&lexBuffer), is_true);
    assert_that(getLexToken(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}
