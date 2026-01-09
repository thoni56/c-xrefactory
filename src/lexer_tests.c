#include <cgreen/cgreen.h>

#include "characterreader.h"
#include "lexembuffer.h"
#include "lexer.h"
#include "log.h"

#include "commons.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "lexem.mock"
#include "misc.mock"
#include "options.mock"
#include "parsing.h"
#include "parsing.mock"
#include "yylex.mock"


static CharacterBuffer characterBuffer;
static LexemBuffer     lexemBuffer;

Describe(Lexer);
BeforeEach(Lexer) {
    parsingConfig.language = LANG_C;
    log_set_level(LOG_ERROR);
    initLexemBuffer(&lexemBuffer);
    always_expect(cachingIsActive, will_return(false));
}
AfterEach(Lexer) {}

Ensure(Lexer, will_signal_false_for_empty_lexbuffer) {
    initCharacterBufferFromFile(&characterBuffer, NULL);
    characterBuffer.fileNumber = 42;

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_false);
}

Ensure(Lexer, can_scan_simple_id) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "id";

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(IDENTIFIER));
}

Ensure(Lexer, can_scan_an_integer) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "43";

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(CONSTANT));
}

Ensure(Lexer, can_scan_a_long_integer) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "43L";

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(LONG_CONSTANT));
}

Ensure(Lexer, can_scan_a_floating_point_number) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "4.3f";

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}

Ensure(Lexer, can_scan_include_next) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "\n#include_next \"file\""; /* Directives must follow \n to be in column 1 */

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    getLexemCodeAndAdvance(&lexemPointer);
    getLexemPositionAndAdvance(&lexemPointer);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(CPP_INCLUDE_NEXT));
    getLexemPositionAndAdvance(&lexemPointer);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(STRING_LITERAL));
}

Ensure(Lexer, can_scan_prefixed_string) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "L\"Hello Marián!\""; /* C11 Wide-character string */

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(STRING_LITERAL));
}

Ensure(Lexer, can_scan_u8_string) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "u8\"Hello Marián!\""; /* C23 UTF-8 string */

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(STRING_LITERAL));
}

Ensure(Lexer, will_not_misinterpret_identifier_starting_with_u8) {
    char *lexemPointer = lexemBuffer.lexemStream;
    char *inputString  = "u8888"; /* NOT a C23 UTF-8 string but an identifier */

    initCharacterBufferFromString(&characterBuffer, inputString);

    assert_that(buildLexemFromCharacters(&characterBuffer, &lexemBuffer, true), is_true);
    assert_that(getLexemCodeAndAdvance(&lexemPointer), is_equal_to(IDENTIFIER));
}
