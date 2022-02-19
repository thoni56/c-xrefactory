#include <cgreen/cgreen.h>

#include "symboltable.h"

#include "memory.h"

#include "misc.mock"
#include "globals.mock"
#include "cxref.mock"           /* For freeOldestOlcx() */


Describe(SymbolTable);
BeforeEach(SymbolTable) {
    initOuterCodeBlock();
}
AfterEach(SymbolTable) {}

static struct symbolTable symtab;

Ensure(SymbolTable, can_init) {
    symbolTableInit(&symtab, 100);
    assert_that(symtab.size, is_equal_to(100));
    for (int i=0; i<100; i++)
        if (symtab.tab[i] != NULL)
            fail_test("allocated symtab entry is not null");
}
