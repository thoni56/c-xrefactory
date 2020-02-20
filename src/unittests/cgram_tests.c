#include <cgreen/cgreen.h>

// Need to include since we are testing static functions
#include "../cgram.c"

#include "semact.mock"
#include "complete.mock"
#include "extract.mock"
#include "misc.mock"
#include "caching.mock"
#include "symbol.mock"
#include "cxref.mock"
#include "yylex.mock"
#include "html.mock"
#include "commons.mock"


Describe(Cgram);
BeforeEach(Cgram){}
AfterEach(Cgram){}

Ensure(Cgram, wtf_expression_returns_expected_value) {
    /* Probably can't find data that can be tested... */
}
