#include <cgreen/cgreen.h>

#include "symtab.h"

#include "misc.mock"

Describe(symTab);
BeforeEach(symTab) {}
AfterEach(symTab) {}

static struct symTab symtab;

Ensure(symTab, can_init) {
    symTabInit(&symtab, 100);
    assert_that(symtab.size, is_equal_to(100));
    for (int i=0; i<100; i++)
        assert_that(symtab.tab[i], is_null);
}
