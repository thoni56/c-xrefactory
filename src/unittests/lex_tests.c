#include <cgreen/cgreen.h>

/* Including the source since we are testing macros */
#include "lex.c"

#include "globals.mock"
#include "commons.mock"
#include "caching.mock"
#include "cxref.mock"
#include "utils.mock"
#include "jslsemact.mock"
#include "characterbuffer.mock"
#include "yylex.mock"
#include "filetab.mock"
#include "filedescriptor.mock"


Describe(Lex);
BeforeEach(Lex) {}
AfterEach(Lex) {}


Ensure(Lex, can_run_an_empty_test) {
}
