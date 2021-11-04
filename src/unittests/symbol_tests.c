#include <cgreen/cgreen.h>

#include "../symbol.h"

#include "globals.mock"
#include "cxref.mock"


Describe(Symbol);
BeforeEach(Symbol) {
    stackMemoryInit();
}
AfterEach(Symbol) {}


Ensure(Symbol, can_create_new_symbol_with_names) {
    char *name = "a_name";
    Symbol *symbol = newSymbol(name, NULL, (Position){.file=-1, .line=0, .col=0});
    assert_that(symbol->name, is_equal_to_string(name));
}

Ensure(Symbol, creates_symbol_with_null_as_next) {
    char *name = "a_name";
    Symbol *symbol = newSymbol(name, NULL, (Position){.file=-1, .line=0, .col=0});
    assert_that(symbol->next, is_null);
}
