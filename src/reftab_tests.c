#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"
#include "memory.h"
#include "proto.h"
#include "reftab.h"

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
    ReferencesItem r = (ReferencesItem){.linkName = "name"};

    int index = addToReferencesTable(&r);
    assert_that(getNextExistingReferencesItem(0), is_equal_to(index));
    assert_that(getNextExistingReferencesItem(index + 1), is_equal_to(-1));
}

Ensure(ReferenceTable, can_retrieve_item_using_index) {
    ReferencesItem r = (ReferencesItem){.linkName = "name"};

    int             index = addToReferencesTable(&r);
    ReferencesItem *item  = getReferencesItem(index);
    assert_that(item->linkName, is_equal_to_string(r.linkName));
}
