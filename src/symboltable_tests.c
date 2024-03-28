#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "symboltable.h"

#include "cxref.mock" /* For freeOldestOlcx() */
#include "globals.mock"
#include "misc.mock"
#include "commons.mock"

#include "stackmemory.h"


Describe(SymbolTable);
BeforeEach(SymbolTable) {
    initOuterCodeBlock();
    initSymbolTable(100);
}
AfterEach(SymbolTable) {}

Ensure(SymbolTable, is_empty_after_init) {
    assert_that(getNextExistingSymbol(0), is_equal_to(-1));
}

Ensure(SymbolTable, can_retrieve_stored_symbol) {
    Symbol symbol = {"symbol"};

    int index = addToSymbolTable(&symbol);
    assert_that(getSymbol(index), is_equal_to(&symbol));
}

Ensure(SymbolTable, can_retrieve_next_existing_symbol) {
    Symbol symbol = {"symbol"};

    int index = addToSymbolTable(&symbol);
    assert_that(getNextExistingSymbol(0), is_equal_to(index));
}

Ensure(SymbolTable, can_lookup_symbol_by_name) {
    Symbol symbol = {"symbol"};

    int index = addToSymbolTable(&symbol);

    int found;
    assert_that(symbolTableIsMember(symbolTable, &symbol, &found, NULL));
    assert_that(found, is_equal_to(index));
}
