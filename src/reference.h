#ifndef REFERENCE_H_INCLUDED
#define REFERENCE_H_INCLUDED

#include "visibility.h"
#include "position.h"
#include "scope.h"
#include "storage.h"
#include "type.h"
#include "usage.h"


#define SCOPES_LN 3

typedef struct reference {
    Usage             usage;
    struct position   position;
    struct reference *next;
} Reference;


typedef struct referenceItem {
    char                     *linkName;
    int                       vApplClass; /* appl class for java virtuals, but also something else ... */
    Type                      type : SYMTYPES_LN;
    Storage                   storage : STORAGES_LN;
    Scope                     scope : SCOPES_LN;
    Visibility                visibility : 2;     /* local/global */
    struct reference         *references;
    struct referenceItem     *next; /* TODO: Link only for hashlist? */
} ReferenceItem;


extern Reference makeReference(Usage usage, Position position, Reference *next);
extern ReferenceItem makeReferenceItem(char *name, int vApplClass, Type type, Storage storage, Scope scope,
                                       Visibility visibility);
extern void fillReference(Reference *reference, Usage usage, Position position, Reference *next);
extern void fillReferenceItem(ReferenceItem *referencesItem, char *name, int vApplClass, Type symType,
                              Storage storage, Scope scope, Visibility visibility);
extern Reference *duplicateReference(Reference *r);
extern void freeReferences(Reference *references);
extern void resetReferenceUsage(Reference *reference, Usage usage);
extern Reference **addToReferenceList(Reference **list, Usage usage, Position pos);
extern Reference *olcxAddReferenceNoUsageCheck(Reference **rlist,
                                               Reference *ref);
extern bool isReferenceInList(Reference *r, Reference *list);
extern Reference *olcxAddReference(Reference **rlist,
                                   Reference *ref);

#endif
