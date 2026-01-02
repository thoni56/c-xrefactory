#include "reference_database.h"
#include "memory.h"
#include "stackmemory.h"
#include "log.h"
#include "globals.h"
#include "position.h"
#include "referenceableitemtable.h"
#include "usage.h"
#include <stdlib.h>

/* Opaque implementation structure */
struct ReferenceDatabase {
    /* For now, just a placeholder */
    int placeholder;
};

/* Context for searching the ReferenceableItemTable by position */
typedef struct {
    Position targetPosition;
    ReferenceableItem *found;
} FindReferenceableContext;

/* Callback for mapOverReferenceableItemTableWithPointer */
static void checkReferenceableItemAtPosition(ReferenceableItem *item, void *contextPtr) {
    FindReferenceableContext *context = (FindReferenceableContext *)contextPtr;

    /* If already found, skip */
    if (context->found != NULL)
        return;

    /* Walk the references list for this item */
    for (Reference *ref = item->references; ref != NULL; ref = ref->next) {
        if (positionsAreEqual(ref->position, context->targetPosition)) {
            context->found = item;
            return;
        }
    }
}

/* Find the ReferenceableItem that has a Reference at the given position */
static ReferenceableItem* findReferenceableItemWithReferenceAt(Position position) {
    FindReferenceableContext context = {
        .targetPosition = position,
        .found = NULL
    };

    mapOverReferenceableItemTableWithPointer(checkReferenceableItemAtPosition, &context);

    return context.found;
}

ReferenceDatabase* createReferenceDatabase(void) {
    ReferenceDatabase* db = malloc(sizeof(ReferenceDatabase));
    if (db) {
        db->placeholder = 42; // Just to initialize something
    }
    return db;
}

void destroyReferenceDatabase(ReferenceDatabase *db) {
    if (db) {
        free(db);
    }
}

/* Extract the definition position from a ReferenceableItem's references */
static Position extractDefinitionPosition(ReferenceableItem *item) {
    /* Walk the references looking for the definition */
    for (Reference *ref = item->references; ref != NULL; ref = ref->next) {
        if (isDefinitionUsage(ref->usage)) {
            return ref->position;
        }
    }
    /* No definition found - return noPosition */
    return noPosition;
}

ReferenceableResult findReferenceableAt(ReferenceDatabase *db, const char *fileName,
                                        Position position) {
    // TODO: This is where we would:
    // 1. Check if fileName needs to be rescanned (file modification time, etc.)
    // 2. Call parseCurrentInputFile() or similar to populate reference tables
    //
    // For now, assume the tables are already populated (by parsing elsewhere)

    (void)db;        // Suppress unused parameter warnings
    (void)fileName;  // fileName would be used for lazy parsing

    /* Find the ReferenceableItem that has a reference at this position */
    ReferenceableItem *item = findReferenceableItemWithReferenceAt(position);

    if (item == NULL) {
        return makeReferenceableFailure();
    }

    /* Extract the definition position from the item's references */
    Position definition = extractDefinitionPosition(item);

    return makeReferenceableResult(item, definition, true);
}

ReferencesResult getReferencesTo(ReferenceDatabase *db, const char *fileName, Position position) {
    // TODO: Automatically check if fileName or its dependencies have changed
    // TODO: Parse/reparse only the files that need updating
    // TODO: Then find all references to the referenceable at position
    return makeReferencesFailure();
}


/* Utility functions */
ReferenceableResult makeReferenceableResult(ReferenceableItem *referenceable, Position definition,
                                            bool found) {
    ReferenceableResult result;
    result.referenceable = referenceable;
    result.definition = definition;
    result.found = found;
    return result;
}

ReferencesResult makeReferencesResult(Reference *references, int count, bool found) {
    ReferencesResult result;
    result.references = references;
    result.count = count;
    result.found = found;
    return result;
}

ReferenceableResult makeReferenceableFailure(void) {
    return makeReferenceableResult(NULL, noPosition, false);
}

ReferencesResult makeReferencesFailure(void) {
    return makeReferencesResult(NULL, 0, false);
}
