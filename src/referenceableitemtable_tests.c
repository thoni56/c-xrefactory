#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"
#include "memory.h"

#include "referenceableitemtable.h"

#include "cxref.mock"
#include "commons.mock"
#include "stackmemory.mock"


Describe(ReferenceableItemTable);
BeforeEach(ReferenceableItemTable) {
    log_set_level(LOG_ERROR);
    initCxMemory(10000);
    initReferenceableItemTable(100);
}
AfterEach(ReferenceableItemTable) {}

Ensure(ReferenceableItemTable, will_return_minus_one_for_no_more_entries) {
    assert_that(getNextExistingReferenceableItem(0), is_equal_to(-1));
}

Ensure(ReferenceableItemTable, will_return_index_to_next_entry) {
    ReferenceableItem r = (ReferenceableItem){.linkName = "name"};

    int index = addToReferenceableItemTable(&r);
    assert_that(getNextExistingReferenceableItem(0), is_equal_to(index));
    assert_that(getNextExistingReferenceableItem(index + 1), is_equal_to(-1));
}

Ensure(ReferenceableItemTable, can_retrieve_item_using_index) {
    ReferenceableItem r = (ReferenceableItem){.linkName = "name"};

    int             index = addToReferenceableItemTable(&r);
    ReferenceableItem *item  = getReferenceableItem(index);
    assert_that(item->linkName, is_equal_to_string(r.linkName));
}

Ensure(ReferenceableItemTable, removes_references_from_chained_items) {
    int SOME_FILE = 42;

    /* Use table size 1 to force hash collision — both items land in bucket 0 */
    initReferenceableItemTable(1);

    Reference ref1 = {.position = {.file = SOME_FILE, .line = 10, .col = 1}, .usage = UsageDefined};
    ReferenceableItem sym1 = {.linkName = "alpha", .references = &ref1};
    addToReferenceableItemTable(&sym1);

    Reference ref2 = {.position = {.file = SOME_FILE, .line = 20, .col = 1}, .usage = UsageDefined};
    ReferenceableItem sym2 = {.linkName = "beta", .references = &ref2};
    addToReferenceableItemTable(&sym2);

    removeReferenceableItemsForFile(SOME_FILE);

    assert_that(sym1.references, is_null);
    assert_that(sym2.references, is_null);
}
