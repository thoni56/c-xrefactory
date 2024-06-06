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
#include "input.mock"
#include "misc.mock"
#include "options.mock"
#include "yylex.mock"


static CharacterBuffer characterBuffer;
static LexemBuffer     lexemBuffer;

Describe(Lexer);
BeforeEach(Lexer) {
    currentLanguage = LANG_C;
    log_set_level(LOG_ERROR);
    initLexemBuffer(&lexemBuffer);
    always_expect(cachingIsActive, will_return(false));
}
AfterEach(Lexer) {}

Ensure(Lexer, will_signal_false_for_empty_lexbuffer) {
    initCharacterBuffer(&characterBuffer, NULL);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer), is_false);
}

Ensure(Lexer, can_scan_a_floating_point_number) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "4.3f";

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer), is_true);
    assert_that(getLexemCodeAt(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}

Ensure(Lexer, can_scan_include_next) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "\n#include_next \"file\""; /* Directives must follow \n to be in column 1 */

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer), is_true);
    getLexemCodeAt(&lexemPointer);
    getLexemPositionAt(&lexemPointer);
    assert_that(getLexemCodeAt(&lexemPointer), is_equal_to(CPP_INCLUDE_NEXT));
    getLexemPositionAt(&lexemPointer);
    assert_that(getLexemCodeAt(&lexemPointer), is_equal_to(STRING_LITERAL));
}
