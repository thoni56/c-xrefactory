#ifndef REFERENCE_H_INCLUDED
#define REFERENCE_H_INCLUDED

#include "position.h"
#include "usage.h"
#include "type.h"
#include "storage.h"

// !!! if you add a pointer to this structure, then update olcxCopyRefList
// A *reference* is a position with a particular usage
#define NO_REFERENCE (Reference) {NO_USAGE, no_Position, NULL)

typedef enum referenceCategory {
    CategoryGlobal,
    CategoryLocal,
    MAX_CATEGORIES
} ReferenceCategory;

typedef enum referenceScope {
    ScopeDefault,
    ScopeGlobal,
    ScopeFile,
    ScopeAuto,
    MAX_SCOPES
} ReferenceScope;

#define SCOPES_LN 3

typedef struct reference {
    struct usage      usage;
    struct position   position;
    struct reference *next;
} Reference;

typedef struct referencesItemBits {
    Type              type : SYMTYPES_LN;
    Storage           storage : STORAGES_LN;
    ReferenceScope    scope : SCOPES_LN;
    Access        accessFlags : 12; /* java access bits */
    ReferenceCategory category : 2;     /* local/global */
} ReferencesItemBits;

// !!! if you add a pointer to this structure, then update olcxCopyRefItem!
typedef struct referencesItem {
    char                     *name;
    unsigned                  fileHash;
    int                       vApplClass; /* appl class for java virtuals */
    int                       vFunClass;  /* fun class for java virtuals */
    struct referencesItemBits bits;
    struct reference         *references;
    struct referencesItem    *next; /* TODO: Link only for hashlist? */
} ReferencesItem;

#endif
