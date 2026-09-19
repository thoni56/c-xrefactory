#include "reference_database.h"

/* Unittests for ReferenceDatabase */
#include <cgreen/cgreen.h>

#include "position.h"
#include "commons.mock"
#include <string.h>
#include "referenceableitemtable.mock"
#include "filetable.mock"


Describe(ReferenceDatabase);

BeforeEach(ReferenceDatabase) {
}

AfterEach(ReferenceDatabase) {
}


Ensure(ReferenceDatabase, can_create_and_destroy_database) {
    ReferenceDatabase *db = createReferenceDatabase();
    assert_that(db, is_not_null);

    destroyReferenceDatabase(db);
}

Ensure(ReferenceDatabase, lookup_automatically_scans_empty_file_and_returns_not_found) {
    // Given I have a ReferenceDatabase
    ReferenceDatabase *db = createReferenceDatabase();
    assert_that(db, is_not_null);

    expect(mapOverReferenceableItemTableWithPointer);

    // When I lookup a referenceable in a file that hasn't been scanned yet
    // (The database should automatically and invisibly scan the file and find nothing)
    Position lookupPosition = {.file = 1, .line = 3, .col = 10};
    ReferenceableResult result = findReferenceableAt(db, "test.c", lookupPosition);

    // Then I should get a failure result
    // (because the database found no referenceable at that position)
    assert_that(result.found, is_false);
    assert_that(result.referenceable, is_null);

    destroyReferenceDatabase(db);
}
