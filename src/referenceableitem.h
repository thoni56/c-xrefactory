#ifndef REFERENCEABLEITEM_H_INCLUDED
#define REFERENCEABLEITEM_H_INCLUDED

#include "reference.h"
#include "scope.h"
#include "storage.h"
#include "type.h"
#include "visibility.h"


// A variable, type, included file, ...
typedef struct referenceableItem {
    char                     *linkName;
    Type                      type : TYPE_BITS;
    int                       includeFileNumber; /* FileNumber for the included file if
                                                  * this is an '#include' Reference item:
                                                  * type = TypeCppInclude */
    Storage                   storage : STORAGES_BITS;
    Scope                     scope : SCOPES_BITS;
    Visibility                visibility : 2;     /* local/global */
    struct reference         *references;
    struct referenceableItem *next; /* TODO: Link only for hashtab */
} ReferenceableItem;


extern ReferenceableItem makeReferenceableItem(char *name, Type type, Storage storage, Scope scope,
                                               Visibility visibility, int includedFileNumber);

#endif
