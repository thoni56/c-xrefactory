#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "memory.h"
#include "proto.h"
#include "reftab.h"
#include "log.h"

#include "cxref.mock"


Describe(ReferenceTable);
BeforeEach(ReferenceTable) {
    log_set_level(LOG_ERROR);
    cxMemoryOverflowHandler(0); /* Implicitly allocate and init cxMemory */
    initReferenceTable(100);
}
AfterEach(ReferenceTable) {}


Ensure(ReferenceTable, will_return_minus_one_for_no_more_entries) {
    assert_that(getNextExistingReferencesItem(0), is_equal_to(-1));
}

Ensure(ReferenceTable, will_return_index_to_next_entry) {
    ReferencesItem r = (ReferencesItem){ .name = "name" };

    int index = addToReferencesTable(&r);
    assert_that(getNextExistingReferencesItem(0), is_equal_to(index));
    assert_that(getNextExistingReferencesItem(index+1), is_equal_to(-1));
}
