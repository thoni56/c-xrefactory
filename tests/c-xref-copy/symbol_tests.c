#include <cgreen/cgreen.h>

#include "symbol.h"

#include "cxref.mock"
#include "globals.mock"
#include "commons.mock"

#include "stackmemory.h"


Describe(Symbol);
BeforeEach(Symbol) {
    initOuterCodeBlock();
}
AfterEach(Symbol) {}

Ensure(Symbol, can_create_new_symbol_with_names) {
    char   *name   = "a_name";
    Symbol *symbol = newSymbol(name, (Position){.file = -1, .line = 0, .col = 0});
    assert_that(symbol->name, is_equal_to_string(name));
}

Ensure(Symbol, creates_symbol_with_null_as_next) {
    char   *name   = "a_name";
    Symbol *symbol = newSymbol(name, (Position){.file = -1, .line = 0, .col = 0});
    assert_that(symbol->next, is_null);
}
