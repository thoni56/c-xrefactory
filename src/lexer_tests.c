#include <cgreen/cgreen.h>

#include "characterreader.h"
#include "lexembuffer.h"
#include "lexer.h"
#include "log.h"

#include "caching.mock"
#include "commons.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jslsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "yylex.mock"
#include "zlib.mock"

Describe(Lexer);
BeforeEach(Lexer) {
    currentLanguage = LANG_C;
    log_set_level(LOG_ERROR);
}
AfterEach(Lexer) {}


Ensure(Lexer, will_signal_false_for_empty_lexbuffer) {
    CharacterBuffer characterBuffer;
    LexemBuffer lexemBuffer;

    initCharacterBuffer(&characterBuffer, NULL);
    initLexemBuffer(&lexemBuffer, &characterBuffer);

    assert_that(getLexemFromLexer(&lexemBuffer), is_false);
}

Ensure(Lexer, can_scan_a_floating_point_number) {
    CharacterBuffer  characterBuffer;
    LexemBuffer      lexemBuffer;
    char            *lexemPointer = lexemBuffer.lexemStream;
    char            *inputString  = "4.3f";

    initLexemBuffer(&lexemBuffer, &characterBuffer);
    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(getLexemFromLexer(&lexemBuffer), is_true);
    assert_that(getLexToken(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}

Ensure(Lexer, can_scan_include_next) {
    CharacterBuffer  charBuffer;
    LexemBuffer      lexemBuffer;
    char            *lexemPointer = lexemBuffer.lexemStream;
    char            *inputString  = "\n#include_next \"file\""; /* Directives must follow \n to be in column 1 */

    initLexemBuffer(&lexemBuffer, &charBuffer);
    initCharacterBufferFromString(lexemBuffer.characterBuffer, inputString);

    assert_that(getLexemFromLexer(&lexemBuffer), is_true);
    getLexToken(&lexemPointer);
    getLexPosition(&lexemPointer);
    assert_that(getLexToken(&lexemPointer), is_equal_to(CPP_INCLUDE_NEXT));
    getLexPosition(&lexemPointer);
    assert_that(getLexToken(&lexemPointer), is_equal_to(STRING_LITERAL));
}
