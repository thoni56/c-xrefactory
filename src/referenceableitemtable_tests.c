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

    /* Use table size 1 to force hash collision - both items land in bucket 0 */
    initReferenceableItemTable(1);

    Reference ref1 = {.position = {.file = SOME_FILE, .line = 10, .col = 1}, .usage = UsageDefined};
    ReferenceableItem sym1 = {.linkName = "alpha", .references = &ref1};
    addToReferenceableItemTable(&sym1);

    Reference ref2 = {.position = {.file = SOME_FILE, .line = 20, .col = 1}, .usage = UsageDefined};
    ReferenceableItem sym2 = {.linkName = "beta", .references = &ref2};
    addToReferenceableItemTable(&sym2);

    removeReferencesForFile(SOME_FILE);

    assert_that(sym1.references, is_null);
    assert_that(sym2.references, is_null);
}

/* A file's include references are edges of the include graph, which only the
 * parse gives for <...> includes */
Ensure(ReferenceableItemTable, can_remove_the_references_of_a_file_except_its_includes) {
    int SOME_FILE = 42;

    Reference symbolReference = {.position = {.file = SOME_FILE, .line = 10, .col = 1}, .usage = UsageDefined};
    ReferenceableItem symbol = {.linkName = "alpha", .type = TypeDefault, .references = &symbolReference};
    addToReferenceableItemTable(&symbol);

    Reference includeReference = {.position = {.file = SOME_FILE, .line = 1, .col = 1}, .usage = UsageUsed};
    ReferenceableItem include = {.linkName = "header.h", .type = TypeCppInclude, .includeFileNumber = 7,
                                 .references = &includeReference};
    addToReferenceableItemTable(&include);

    removeReferencesForFileExceptIncludes(SOME_FILE);

    assert_that(symbol.references, is_null);
    assert_that(include.references, is_equal_to(&includeReference));
}

/* A config change is a change of every file, and the include references stay
 * for the same reason as a changed CU's */
Ensure(ReferenceableItemTable, can_remove_every_reference_except_the_includes) {
    Reference firstReference = {.position = {.file = 42, .line = 10, .col = 1}, .usage = UsageDefined};
    ReferenceableItem first = {.linkName = "alpha", .type = TypeDefault, .references = &firstReference};
    addToReferenceableItemTable(&first);

    Reference secondReference = {.position = {.file = 43, .line = 3, .col = 1}, .usage = UsageUsed};
    ReferenceableItem second = {.linkName = "beta", .type = TypeDefault, .references = &secondReference};
    addToReferenceableItemTable(&second);

    Reference includeReference = {.position = {.file = 42, .line = 1, .col = 1}, .usage = UsageUsed};
    ReferenceableItem include = {.linkName = "header.h", .type = TypeCppInclude, .includeFileNumber = 7,
                                 .references = &includeReference};
    addToReferenceableItemTable(&include);

    removeAllReferencesExceptIncludes();

    assert_that(first.references, is_null);
    assert_that(second.references, is_null);
    assert_that(include.references, is_equal_to(&includeReference));
}
