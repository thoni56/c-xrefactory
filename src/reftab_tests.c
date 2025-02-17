#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"
#include "memory.h"

#include "reftab.h"

#include "cxref.mock"
#include "commons.mock"
#include "stackmemory.mock"


Describe(ReferenceTable);
BeforeEach(ReferenceTable) {
    log_set_level(LOG_ERROR);
    initCxMemory(1000);
    initReferenceTable(100);
}
AfterEach(ReferenceTable) {}

Ensure(ReferenceTable, will_return_minus_one_for_no_more_entries) {
    assert_that(getNextExistingReferenceItem(0), is_equal_to(-1));
}

Ensure(ReferenceTable, will_return_index_to_next_entry) {
    ReferenceItem r = (ReferenceItem){.linkName = "name"};

    int index = addToReferencesTable(&r);
    assert_that(getNextExistingReferenceItem(0), is_equal_to(index));
    assert_that(getNextExistingReferenceItem(index + 1), is_equal_to(-1));
}

Ensure(ReferenceTable, can_retrieve_item_using_index) {
    ReferenceItem r = (ReferenceItem){.linkName = "name"};

    int             index = addToReferencesTable(&r);
    ReferenceItem *item  = getReferenceItem(index);
    assert_that(item->linkName, is_equal_to_string(r.linkName));
}
