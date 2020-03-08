#include <cgreen/cgreen.h>

#include "../symbol.h"

#include "../proto.h"           /* For S_position */

void *stackMemoryAlloc(int size) { return malloc(size); }

Describe(Symbol);
BeforeEach(Symbol) {}
AfterEach(Symbol) {}


Ensure(Symbol, can_create_new_symbol_with_names) {
    char *name = "a_name";
    Symbol *symbol = newSymbol(name, NULL, (S_position){.file=-1, .line=0, .col=0});
    assert_that(symbol->name, is_equal_to_string(name));
}

Ensure(Symbol, creates_symbol_with_null_as_next) {
    char *name = "a_name";
    Symbol *symbol = newSymbol(name, NULL, (S_position){.file=-1, .line=0, .col=0});
    assert_that(symbol->next, is_null);
}
