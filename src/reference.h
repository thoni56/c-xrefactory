#ifndef REFERENCE_H_INCLUDED
#define REFERENCE_H_INCLUDED

#include "visibility.h"
#include "position.h"
#include "scope.h"
#include "storage.h"
#include "type.h"
#include "usage.h"


#define SCOPES_BITS 3

// An occurence of a referenceItem
typedef struct reference {
    struct position   position;
    Usage             usage;
    struct reference *next;
} Reference;

// A variable, type, included file, ...
typedef struct referenceItem {
    char                     *linkName;
    int                       includedFileNumber; /* This probably was application class
                                           * for java virtuals, but for C it
                                           * seems to be the fileNumber for the
                                           * included file if this is an
                                           * '#include' Reference item */
    Type                      type : SYMTYPES_BITS;
    Storage                   storage : STORAGES_BITS;
    Scope                     scope : SCOPES_BITS;
    Visibility                visibility : 2;     /* local/global */
    struct reference         *references;
    struct referenceItem     *next; /* TODO: Link only for hashlist? */
} ReferenceItem;


extern Reference *newReference(Position position, Usage usage, Reference *next);
extern Reference makeReference(Position position, Usage usage, Reference *next);
extern Reference *duplicateReferenceInCxMemory(Reference *r);
extern void freeReferences(Reference *references);
extern void resetReferenceUsage(Reference *reference, Usage usage);
extern Reference **addToReferenceList(Reference **list, Position pos, Usage usage);
extern bool isReferenceInList(Reference *r, Reference *list);
extern Reference *addReferenceToList(Reference **rlist,
                                   Reference *ref);
extern ReferenceItem makeReferenceItem(char *name, Type type, Storage storage, Scope scope,
                                       Visibility visibility, int includedFileNumber);
extern int fileNumberOfReference(Reference reference);

#endif
