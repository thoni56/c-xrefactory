#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "options.h"
#include "position.h"
#include "semact.h"

#include "caching.mock"
#include "classcaster.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "jslsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "server.h"
#include "symbol.mock"
#include "symboltable.mock"
#include "typemodifier.mock"
#include "yylex.mock"

Describe(Semact);
BeforeEach(Semact) {}
AfterEach(Semact) {}

Ensure(Semact, can_generate_message_for_no_such_field) {
    options.debug = true;
    expect(errorMessage, when(message, is_equal_to_string("Field/member 'field' not found")));
    noSuchFieldError("field");
}

Ensure(Semact, can_capture_positions_for_empty_declarator_params) {
    /* void function() ... */
    Position lpar = {.file = 42, .line = 5, .col = 20};
    Position rpar = {.file = 42, .line = 5, .col = 21};
    Symbol symbol = {.pos = {.file=42, .line = 5, .col = 12}};

    options.mode = ServerMode;
    options.serverOperation = OLO_GOTO_PARAM_NAME;
    cxRefPosition = symbol.pos;

    handleDeclaratorParamPositions(&symbol, &lpar, NULL, &rpar, false);

    assert_that(positionsAreEqual(parameterBeginPosition, lpar));
    assert_that(positionsAreEqual(parameterEndPosition, lpar));
}
