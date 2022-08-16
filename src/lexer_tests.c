#include <cgreen/cgreen.h>

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

protected void shiftAnyRemainingLexems(LexemBuffer *lb);

Ensure(Lexer, can_shift_remaining_lexems) {
    LexemBuffer lb = { .end = lb.lexemStream, .next = lb.lexemStream };
    char *test_string = "abcdefg";

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.next, is_equal_to(lb.lexemStream));
    assert_that(lb.end, is_equal_to(lb.lexemStream));

    lb.lexemStream[1] = '1';
    lb.next = &lb.lexemStream[1];
    lb.end  = &lb.lexemStream[2];

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.next, is_equal_to(lb.lexemStream));
    assert_that(lb.end, is_equal_to(&lb.lexemStream[1]));
    assert_that(lb.lexemStream[0], is_equal_to('1'));

    strcpy(&lb.lexemStream[2], test_string);
    lb.next = &lb.lexemStream[2];
    lb.end  = &lb.lexemStream[9];

    shiftAnyRemainingLexems(&lb);

    assert_that(lb.next, is_equal_to(lb.lexemStream));
    assert_that(lb.end, is_equal_to(&lb.lexemStream[strlen(test_string)]));
    assert_that(strncmp(lb.lexemStream, test_string, strlen(test_string)), is_equal_to(0));
}

Ensure(Lexer, will_signal_false_for_empty_lexbuffer) {
    LexemBuffer lexemBuffer;

    cache.cachingActive = false; /* ?? */

    lexemBuffer.next = lexemBuffer.end = lexemBuffer.lexemStream;
    lexemBuffer.ringIndex = 0;

    initCharacterBuffer(&lexemBuffer.buffer, NULL);

    assert_that(getLexemFromLexer(&lexemBuffer), is_false);
}

Ensure(Lexer, can_scan_a_floating_point_number) {
    LexemBuffer      lexemBuffer;
    char            *lexemPointer = lexemBuffer.lexemStream;
    CharacterBuffer *charBuffer   = &lexemBuffer.buffer;
    char            *inputString  = "4.3f";

    cache.cachingActive = false; /* ?? */

    initLexemBuffer(&lexemBuffer, NULL);
    charBuffer->fileNumber = 0;

    initCharacterBufferFromString(&lexemBuffer.buffer, inputString);

    assert_that(getLexemFromLexer(&lexemBuffer), is_true);
    assert_that(getLexToken(&lexemPointer), is_equal_to(DOUBLE_CONSTANT));
}

Ensure(Lexer, can_scan_include_next) {
    LexemBuffer      lexemBuffer;
    char            *lexemPointer = lexemBuffer.lexemStream;
    CharacterBuffer *charBuffer   = &lexemBuffer.buffer;
    char            *inputString  = "\n#include_next \"file\""; /* Directives must follow \n to be in column 1 */

    cache.cachingActive = false; /* ?? */

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
