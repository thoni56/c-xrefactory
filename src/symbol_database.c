#include "symbol_database.h"
#include "memory.h"
#include "stackmemory.h"
#include "log.h"
#include "globals.h"
#include <stdlib.h>

/* Opaque implementation structure */
struct SymbolDatabase {
    /* For now, just a placeholder */
    int placeholder;
};

SymbolDatabase* createSymbolDatabase(void) {
    SymbolDatabase* db = malloc(sizeof(SymbolDatabase));
    if (db) {
        db->placeholder = 42; // Just to initialize something
    }
    return db;
}

void destroySymbolDatabase(SymbolDatabase *db) {
    if (db) {
        free(db);
    }
}

SymbolLookupResult lookupSymbol(SymbolDatabase *db, const char *fileName, Position pos) {
    // TODO: Automatically check if fileName or its dependencies have changed
    // TODO: Parse/reparse only the files that need updating
    // TODO: Then perform the actual symbol lookup
    return makeSymbolLookupFailure();
}

ReferenceLookupResult getReferences(SymbolDatabase *db, const char *fileName, Position pos) {
    // TODO: Automatically check if fileName or its dependencies have changed  
    // TODO: Parse/reparse only the files that need updating
    // TODO: Then find all references to the symbol at pos
    return makeReferenceLookupFailure();
}


/* Utility functions */
SymbolLookupResult makeSymbolLookupResult(Symbol *symbol, Position definition, bool found) {
    SymbolLookupResult result;
    result.symbol = symbol;
    result.definition = definition;
    result.found = found;
    return result;
}

ReferenceLookupResult makeReferenceLookupResult(Reference *references, int count, bool found) {
    ReferenceLookupResult result;
    result.references = references;
    result.count = count;
    result.found = found;
    return result;
}

SymbolLookupResult makeSymbolLookupFailure(void) {
    return makeSymbolLookupResult(NULL, noPosition, false);
}

ReferenceLookupResult makeReferenceLookupFailure(void) {
    return makeReferenceLookupResult(NULL, 0, false);
}