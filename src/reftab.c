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
        && e1->category==e2->category
        && e1->vApplClass==e2->vApplClass
        && strcmp(e1->linkName, e2->linkName)==0;
}


#define HASH_FUN(element) (hashFun(element->linkName) + (unsigned)element->vFunClass)
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
