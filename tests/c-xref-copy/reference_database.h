#ifndef REFERENCE_DATABASE_H_INCLUDED
#define REFERENCE_DATABASE_H_INCLUDED

#include "position.h"
#include "referenceableitem.h"


/* Opaque handle - implementation details hidden */
typedef struct ReferenceDatabase ReferenceDatabase;

/* Result structure for finding a referenceable at a position */
typedef struct {
    ReferenceableItem *referenceable; /* The found item, NULL if not found */
    Position definition;              /* Position of definition */
    bool found;                       /* Whether lookup was successful */
} ReferenceableResult;

/* Result structure for getting references to a referenceable */
typedef struct {
    Reference *references;    /* Linked list of references */
    int count;               /* Number of references found */
    bool found;              /* Whether lookup was successful */
} ReferencesResult;


/* Factory function - creates database using current implementation */
extern ReferenceDatabase* createReferenceDatabase(void);

/* Core API - implementation independent interface */
extern ReferenceableResult findReferenceableAt(ReferenceDatabase *db, const char *fileName, Position pos);
extern ReferencesResult getReferencesTo(ReferenceDatabase *db, const char *fileName, Position pos);
extern void destroyReferenceDatabase(ReferenceDatabase *db);

/* Utility functions for results */
extern ReferenceableResult makeReferenceableResult(ReferenceableItem *referenceable, Position definition, bool found);
extern ReferencesResult makeReferencesResult(Reference *references, int count, bool found);

/* Convenience functions for common cases */
extern ReferenceableResult makeReferenceableFailure(void);
extern ReferencesResult makeReferencesFailure(void);

#endif
