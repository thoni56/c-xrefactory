#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "symboltable.h"

#include "memory.h"

#include "misc.mock"
#include "globals.mock"
#include "cxref.mock"           /* For freeOldestOlcx() */


Describe(SymbolTable);
BeforeEach(SymbolTable) {
    initSymbolTable();
    initOuterCodeBlock();
}
AfterEach(SymbolTable) {}


Ensure(SymbolTable, is_empty_after_init) {
    assert_that(getNextExistingSymbol(0), is_equal_to(-1));
}


Ensure(SymbolTable, can_retrieve_stored_symbol) {

}
