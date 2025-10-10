#include <cgreen/cgreen.h>

#include "symbol_database.h"
#include "commons.mock"

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