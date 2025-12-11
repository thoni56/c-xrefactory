#ifndef SYMBOL_DATABASE_H_INCLUDED
#define SYMBOL_DATABASE_H_INCLUDED

#include "position.h"
#include "symbol.h"
#include "referenceableitem.h"

/* Opaque handle - implementation details hidden */
typedef struct SymbolDatabase SymbolDatabase;

/* Result structure for symbol lookups */
typedef struct {
    Symbol *symbol;           /* The found symbol, NULL if not found */
    Position definition;      /* Position of symbol definition */
    bool found;              /* Whether lookup was successful */
} SymbolLookupResult;

/* Result structure for reference lookups */
typedef struct {
    Reference *references;    /* Linked list of references */
    int count;               /* Number of references found */
    bool found;              /* Whether lookup was successful */
} ReferenceLookupResult;

/* Factory function - creates database using current implementation */
extern SymbolDatabase* createSymbolDatabase(void);

/* Core API - implementation independent interface */
extern SymbolLookupResult lookupSymbol(SymbolDatabase *db, const char *fileName, Position pos);
extern ReferenceLookupResult getReferences(SymbolDatabase *db, const char *fileName, Position pos);
extern void destroySymbolDatabase(SymbolDatabase *db);

/* Utility functions for results */
extern SymbolLookupResult makeSymbolLookupResult(Symbol *symbol, Position definition, bool found);
extern ReferenceLookupResult makeReferenceLookupResult(Reference *references, int count, bool found);

/* Convenience functions for common cases */
extern SymbolLookupResult makeSymbolLookupFailure(void);
extern ReferenceLookupResult makeReferenceLookupFailure(void);

#endif
