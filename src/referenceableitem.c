#include "referenceableitem.h"

#include <stdio.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"


ReferenceableItem makeReferenceableItem(char *name, Type type, Storage storage, Scope scope,
                                        Visibility visibility, int includedFileNumber) {
    ReferenceableItem item;

    item.linkName = name;
    item.type = type;
    item.storage = storage;
    item.scope = scope;
    item.visibility = visibility;
    if (includedFileNumber != NO_FILE_NUMBER)
        assert(type == TypeCppInclude);
    item.includedFileNumber = includedFileNumber;

    item.references = NULL;
    item.next = NULL;

    return item;
}
