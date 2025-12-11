#include "referenceableitem.h"

#include <stdio.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"


ReferenceableItem makeReferenceableItem(char *name, Type type, Storage storage, Scope scope,
                                        Visibility visibility, int includeFile) {
    ReferenceableItem item;

    item.linkName = name;
    item.type = type;
    item.storage = storage;
    item.scope = scope;
    item.visibility = visibility;
    if (includeFile != NO_FILE_NUMBER) /* Only '#include' can have a file number */
        assert(type == TypeCppInclude);
    item.includeFile = includeFile;

    item.references = NULL;
    item.next = NULL;

    return item;
}
