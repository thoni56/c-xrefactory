#include "includegraph.h"

#include <stdio.h>
#include "referenceableitemtable.h"
#include "type.h"


typedef struct {
    int  includerFileNumber;
    int *fileNumbers;
    int  maxFileNumbers;
    int  count;
} IncludedFiles;


static bool isCollected(IncludedFiles *included, int fileNumber) {
    for (int i = 0; i < included->count && i < included->maxFileNumbers; i++)
        if (included->fileNumbers[i] == fileNumber)
            return true;
    return false;
}

static void collectIfIncludedBy(ReferenceableItem *item, void *includedFilesP) {
    if (item->type != TypeCppInclude)
        return;
    IncludedFiles *included = includedFilesP;
    for (Reference *r = item->references; r != NULL; r = r->next)
        if (r->position.file == included->includerFileNumber && r->usage != UsageDefined
            && !isCollected(included, item->includeFileNumber)) {
            if (included->count < included->maxFileNumbers)
                included->fileNumbers[included->count] = item->includeFileNumber;
            included->count++;
        }
}

int collectIncludeClosure(int fileNumber, int fileNumbers[], int maxFileNumbers) {
    IncludedFiles included = {.includerFileNumber = fileNumber, .fileNumbers = fileNumbers,
                              .maxFileNumbers = maxFileNumbers};
    mapOverReferenceableItemTableWithPointer(collectIfIncludedBy, &included);
    for (int i = 0; i < included.count && i < maxFileNumbers; i++) {
        included.includerFileNumber = fileNumbers[i];
        mapOverReferenceableItemTableWithPointer(collectIfIncludedBy, &included);
    }
    return included.count;
}
