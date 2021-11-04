#include <cgreen/cgreen.h>

#include "olcxtab.h"
#include "misc.mock"
#include "globals.mock"
#include "cxref.mock"


Describe(olcxTab);
BeforeEach(olcxTab) {
    stackMemoryInit();
}
AfterEach(olcxTab) {}

static struct olcxTab table;

Ensure(olcxTab, can_init) {
    olcxTabInit(&table, 100);
    assert_that(table.size, is_equal_to(100));
    for (int i=0; i<100; i++)
        assert_that(table.tab[i], is_null);
}
