#include <cgreen/cgreen.h>

#include "symbol_database.h"
#include "position.h"
#include "commons.mock"
#include <string.h>

Describe(SymbolDatabase);

BeforeEach(SymbolDatabase) {
}

AfterEach(SymbolDatabase) {
}

Ensure(SymbolDatabase, can_create_and_destroy_database) {
    SymbolDatabase *db = createSymbolDatabase();
    assert_that(db, is_not_null);
    
    destroySymbolDatabase(db);
}

Ensure(SymbolDatabase, lookup_automatically_scans_empty_file_and_returns_not_found) {
    // Given I have a SymbolDatabase
    SymbolDatabase *db = createSymbolDatabase();
    assert_that(db, is_not_null);
    
    // When I lookup a symbol in a file that hasn't been scanned yet
    // (The database should automatically scan the file and find nothing)
    Position lookupPos = {.file = 1, .line = 3, .col = 10};
    SymbolLookupResult result = lookupSymbol(db, "test.c", lookupPos);
    
    // Then I should get a failure result 
    // (because the database scanned the file but found no symbol at that position)
    assert_that(result.found, is_false);
    assert_that(result.symbol, is_null);
    
    destroySymbolDatabase(db);
}
