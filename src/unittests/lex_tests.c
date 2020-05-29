#include <cgreen/cgreen.h>

/* Including the source since we are testing macros */
#include "lex.c"

#include "globals.mock"
#include "commons.mock"
#include "caching.mock"
#include "cxref.mock"
#include "utils.mock"
#include "jslsemact.mock"
#include "yylex.mock"
#include "filetab.mock"
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

    assert_that(getLexBuf(&lexBuffer), is_false);
}
