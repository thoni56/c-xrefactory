#include <string.h>

#include "head.h"

#define IN_REFTAB_C
#include "reftab.h"

#include "memory.h"               /* For XX_ALLOCC */
#include "log.h"


static ReferenceTable referenceTable;

static bool equalReferenceItems(ReferenceItem *e1, ReferenceItem *e2) {
    return e1->type==e2->type
        && e1->storage==e2->storage
        && e1->visibility==e2->visibility
        && e1->includedFileNumber==e2->includedFileNumber
        && strcmp(e1->linkName, e2->linkName)==0;
}


#define HASH_FUN(element) (hashFun(element->linkName) + (unsigned)element->includedFileNumber)
#define HASH_ELEM_EQUAL(e1,e2) equalReferenceItems(e1, e2)

#include "hashlist.tc"


void initReferenceTable(int size) {
    // We want this in cx_memory, so can't use refTabInit() b.c it allocates in StackMemory
    referenceTable.tab = cxAlloc(size*sizeof(ReferenceItem *));
    refTabNoAllocInit(&referenceTable, size);
}

ReferenceItem *getReferenceItem(int index) {
    assert(index < referenceTable.size);
    assert(referenceTable.tab[index]);
    return referenceTable.tab[index];
}

int getNextExistingReferenceItem(int index) {
    for (int i=index; i < referenceTable.size; i++)
        if (referenceTable.tab[i] != NULL)
            return i;
    return -1;
}

int addToReferencesTable(ReferenceItem *referenceItem) {
    return refTabAdd(&referenceTable, referenceItem);
}

void pushReferenceItem(ReferenceItem *element, int position) {
    refTabPush(&referenceTable, element, position);
}

void setReferenceItem(int index, ReferenceItem *item) {
    referenceTable.tab[index] = item;
}

bool isMemberInReferenceTable(ReferenceItem *element, int *position, ReferenceItem **foundMemberPointer) {
    return refTabIsMember(&referenceTable, element, position, foundMemberPointer);
}

void mapOverReferenceTable(void (*fun)(ReferenceItem *)) {
    ENTER();
    refTabMap(&referenceTable, fun);
    LEAVE();
}

void mapOverReferenceTableWithPointer(void (*fun)(ReferenceItem *, void *), void *pointer) {
    refTabMapWithPointer(&referenceTable, fun, pointer);
}

void mapOverReferenceTableWithIndex(void (*fun)(int index)) {
    refTabMapWithIndex(&referenceTable, fun);
}

void removeReferencesForFile(int fileNumber) {
    log_trace("removing all references for file number %d", fileNumber);

    for (int i = 0; i < referenceTable.size; i++) {
        ReferenceItem *item = referenceTable.tab[i];
        if (item == NULL)
            continue;

        /* Remove references matching fileNumber from this item's reference list */
        Reference **refP = &(item->references);
        while (*refP != NULL) {
            if ((*refP)->position.file == fileNumber) {
                log_trace("removing reference to %s at %d:%d", item->linkName,
                         (*refP)->position.line, (*refP)->position.col);
                *refP = (*refP)->next;  /* Unlink this reference */
            } else {
                refP = &((*refP)->next);  /* Move to next */
            }
        }
    }
}
