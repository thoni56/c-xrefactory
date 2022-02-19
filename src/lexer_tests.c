#include <cgreen/cgreen.h>

#include "lexer.h"
#include "log.h"

#include "globals.mock"
#include "options.mock"
#include "commons.mock"
#include "caching.mock"
#include "utils.mock"
#include "jslsemact.mock"
#include "yylex.mock"
#include "filetable.mock"
#include "filedescriptor.mock"
#include "zlib.mock"
#include "fileio.mock"


Describe(Lexer);
BeforeEach(Lexer) {
    s_language = LANG_C;
    log_set_level(LOG_ERROR);
}
AfterEach(Lexer) {}


Ensure(Lexer, will_signal_false_for_empty_lexbuffer) {
    LexemBuffer lexemBuffer;

    s_cache.activeCache = false; /* ?? */

    lexemBuffer.next = NULL;
    lexemBuffer.end = NULL;
    lexemBuffer.index = 0;

    initCharacterBuffer(&lexemBuffer.buffer, NULL);

    assert_that(getLexemFromLexer(&lexemBuffer), is_false);
}

Ensure(Lexer, can_scan_a_floating_point_number) {
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

Ensure(Lexer, can_scan_include_next) {
    LexemBuffer lexemBuffer;
    char *lexemPointer = lexemBuffer.lexemStream;
    CharacterBuffer *charBuffer = &lexemBuffer.buffer;
    char *inputString = "\n#include_next \"file\""; /* Directives must follow \n to be in column 1 */

    s_cache.activeCache = false; /* ?? */

    initLexemBuffer(&lexemBuffer, NULL);
    charBuffer->fileNumber = 0;

    initCharacterBufferFromString(&lexemBuffer.buffer, inputString);

    assert_that(getLexemFromLexer(&lexemBuffer), is_true);
    getLexToken(&lexemPointer);
    getLexPosition(&lexemPointer);
    assert_that(getLexToken(&lexemPointer), is_equal_to(CPP_INCLUDE_NEXT));
    getLexPosition(&lexemPointer);
    assert_that(getLexToken(&lexemPointer), is_equal_to(STRING_LITERAL));
}
