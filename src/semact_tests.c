#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "globals.h"
#include "options.h"
#include "position.h"
#include "semact.h"

#include "caching.mock"
#include "counters.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filetable.mock"
#include "filedescriptor.mock"
#include "globals.mock"
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
    noSuchMemberError("field");
}

Ensure(Semact, can_capture_positions_for_empty_parameter_list) {
    /*
      | declarator2 '(' ')'                               {
        handleDeclaratorParamPositions($1.data, &$2.data, NULL, &$3.data, 0);
    */

    /* void function() ... */
    Symbol symbol = {.pos = {.file=42, .line = 5, .col = 12}};
    Position lpar = {.file = 42, .line = 5, .col = 20};
    Position rpar = {.file = 42, .line = 5, .col = 21};

    options.mode = ServerMode;
    options.serverOperation = OLO_GOTO_PARAM_NAME;
    cxRefPosition = symbol.pos;

    /* No position list (commas) for no parameters */
    handleDeclaratorParamPositions(&symbol, lpar, NULL, rpar, false, false);

    assert_that(positionsAreEqual(parameterBeginPosition, lpar));
    assert_that(positionsAreEqual(parameterEndPosition, lpar));
    assert_that(!parameterListIsVoid);
    assert_that(parameterCount, is_equal_to(0));
}

Ensure(Semact, can_capture_positions_for_one_parameter) {
    /* void function(int arg) ... */
    Symbol symbol = {.pos = {.file=42, .line = 5, .col = 12}};
    Position lpar = {.file = 42, .line = 5, .col = 20};
    Position rpar = {.file = 42, .line = 5, .col = 28};

    options.mode = ServerMode;
    options.serverOperation = OLO_GOTO_PARAM_NAME;
    options.olcxGotoVal = 1;
    cxRefPosition = symbol.pos;

    handleDeclaratorParamPositions(&symbol, lpar, NULL, rpar, true, false);

    assert_that(positionsAreEqual(parameterBeginPosition, lpar));
    assert_that(positionsAreEqual(parameterEndPosition, rpar));
    assert_that(!parameterListIsVoid);
    assert_that(parameterCount, is_equal_to(1));
}

Ensure(Semact, can_capture_positions_for_two_parameters) {
    /* void function(int arg, char *string) ... */
    Symbol symbol = {.pos = {.file=42, .line = 5, .col = 12}};
    Position lpar = {.file = 42, .line = 5, .col = 20};
    Position rpar = {.file = 42, .line = 5, .col = 42};
    Position comma = {.file = 5, .line = 5, .col = 28};
    PositionList commas = { .position = comma, .next = NULL };

    options.mode = ServerMode;
    options.serverOperation = OLO_GOTO_PARAM_NAME;
    options.olcxGotoVal = 1;
    cxRefPosition = symbol.pos;

    handleDeclaratorParamPositions(&symbol, lpar, &commas, rpar, true, false);

    assert_that(positionsAreEqual(parameterBeginPosition, lpar));
    assert_that(positionsAreEqual(parameterEndPosition, comma));
    assert_that(!parameterListIsVoid);
    assert_that(parameterCount, is_equal_to(2));
}

Ensure(Semact, can_capture_positions_for_void_parameter_list) {
    /* void function(void) ... */
    Symbol symbol = {.pos = {.file=42, .line = 5, .col = 12}};
    Position lpar = {.file = 42, .line = 5, .col = 20};
    Position rpar = {.file = 42, .line = 5, .col = 25};

    options.mode = ServerMode;
    options.serverOperation = OLO_GOTO_PARAM_NAME;
    options.olcxGotoVal = 1;
    cxRefPosition = symbol.pos;

    handleDeclaratorParamPositions(&symbol, lpar, NULL, rpar, true, true);

    assert_that(positionsAreEqual(parameterBeginPosition, lpar));
    assert_that(positionsAreEqual(parameterEndPosition, rpar));
    assert_that(parameterListIsVoid);
    assert_that(parameterCount, is_equal_to(0));
}
