#include <cgreen/cgreen.h>

#include "symboltable.h"
#include "memory.mock"
#include "misc.mock"


Describe(symbolTable);
BeforeEach(symbolTable) {}
AfterEach(symbolTable) {}

static struct symbolTable symtab;

Ensure(symbolTable, can_init) {
    symbolTableInit(&symtab, 100);
    assert_that(symtab.size, is_equal_to(100));
    for (int i=0; i<100; i++)
        if (symtab.tab[i] != NULL)
            fail_test("allocated symtab entry is not null");
}
