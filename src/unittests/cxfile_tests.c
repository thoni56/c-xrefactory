#include <cgreen/cgreen.h>

/* Must #include it since we are (for now) unittesting an internal macro (ScanInt) */
#include "cxfile.c"

#include "globals.mock"
#include "olcxtab.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "html.mock"
#include "reftab.mock"
#include "filetab.mock"
#include "options.mock"
#include "yylex.mock"
#include "lex.mock"
#include "editor.mock"
#include "memory.mock"
#include "characterbuffer.mock"
#include "classhierarchy.mock"

#include "enumTxt.c"

Describe(CxFile);
BeforeEach(CxFile) {
    log_set_console_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(CxFile) {}

static CharacterBuffer characterBuffer;

Ensure(CxFile, can_scan_int) {
    CharacterBuffer *cb = &characterBuffer;
    char next = ' ';
    char *characters;
    char *end;
    int result;

    characters = cb->chars;
    strcpy(cb->chars, "123");
    cb->end = &cb->chars[strlen("123")];
    end = cb->end;

    expect(fillBuffer,
           when(buffer, is_equal_to_hex(&characterBuffer)));

    ScanInt(next, characters, end, &characterBuffer, result);

    assert_that(result, is_equal_to(123));
}


Ensure(CxFile, can_get_char) {
    CharacterBuffer *cb = &characterBuffer;
    char next = ' ';
    char *characters;
    char *end;

    characters = cb->chars;
    strcpy(cb->chars, "123");
    cb->end = &cb->chars[strlen("123")];
    end = cb->end;

    GetChar(next, characters, end, &characterBuffer);
    assert_that(next, is_equal_to('1'));

    GetChar(next, characters, end, &characterBuffer);
    assert_that(next, is_equal_to('2'));

    GetChar(next, characters, end, &characterBuffer);
    assert_that(next, is_equal_to('3'));
}
