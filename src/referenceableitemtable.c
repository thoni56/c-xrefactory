#include <string.h>

#include "head.h"
#include "referenceableitem.h"

#define IN_REFTAB_C
#include "referenceableitemtable.h"

#include "memory.h"               /* For XX_ALLOCC */
#include "log.h"


static ReferenceableItemTable referenceableItemTable;

static bool equalReferenceableItems(ReferenceableItem *e1, ReferenceableItem *e2) {
    return e1->type==e2->type
        && e1->storage==e2->storage
        && e1->visibility==e2->visibility
        && e1->includeFile==e2->includeFile
        && strcmp(e1->linkName, e2->linkName)==0;
}


#define HASH_FUN(element) (hashFun(element->linkName) + (unsigned)element->includeFile)
#define HASH_ELEM_EQUAL(e1,e2) equalReferenceableItems(e1, e2)

#include "hashlist.tc"


void initReferenceableItemTable(int size) {
    // We want this in cx_memory, so can't use refTabInit() b.c it allocates in StackMemory
    referenceableItemTable.tab = cxAlloc(size*sizeof(ReferenceableItem *));
    referenceableItemTableNoAllocInit(&referenceableItemTable, size);
}

ReferenceableItem *getReferenceableItem(int index) {
    assert(index < referenceableItemTable.size);
    assert(referenceableItemTable.tab[index]);
    return referenceableItemTable.tab[index];
}

int getNextExistingReferenceableItem(int index) {
    for (int i=index; i < referenceableItemTable.size; i++)
        if (referenceableItemTable.tab[i] != NULL)
            return i;
    return -1;
}

int addToReferenceableItemTable(ReferenceableItem *referenceableItem) {
    return referenceableItemTableAdd(&referenceableItemTable, referenceableItem);
}

void pushReferenceableItem(ReferenceableItem *element, int position) {
    referenceableItemTablePush(&referenceableItemTable, element, position);
}

void setReferenceableItem(int index, ReferenceableItem *item) {
    referenceableItemTable.tab[index] = item;
}

bool isMemberInReferenceableItemTable(ReferenceableItem *element, int *position, ReferenceableItem **foundMemberPointer) {
    return referenceableItemTableIsMember(&referenceableItemTable, element, position, foundMemberPointer);
}

void mapOverReferenceableItemTable(void (*fun)(ReferenceableItem *)) {
    ENTER();
    referenceableItemTableMap(&referenceableItemTable, fun);
    LEAVE();
}

void mapOverReferenceableItemTableWithPointer(void (*fun)(ReferenceableItem *, void *), void *pointer) {
    referenceableItemTableMapWithPointer(&referenceableItemTable, fun, pointer);
}

void mapOverReferenceableItemTableWithIndex(void (*fun)(int index)) {
    referenceableItemTableMapWithIndex(&referenceableItemTable, fun);
}

void removeReferenceableItemsForFile(int fileNumber) {
    log_trace("removing all references for file number %d", fileNumber);

    for (int i = 0; i < referenceableItemTable.size; i++) {
        ReferenceableItem *item = referenceableItemTable.tab[i];
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
