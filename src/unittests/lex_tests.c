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


xEnsure(Lex, can_run_initial_test) {
    S_lexBuf lexBuffer;

    s_cache.activeCache = false;
    initCharacterBuffer(&lexBuffer.buffer, NULL, 0);

    getLexBuf(&lexBuffer);
}
