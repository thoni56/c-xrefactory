#include "proto.h"
#define IN_REFTAB_C
#include "reftab.h"


#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */


static ReferenceTable referenceTable;

static bool equalReferenceItems(ReferencesItem *e1, ReferencesItem *e2) {
    return e1->type==e2->type
        && e1->storage==e2->storage
        && e1->category==e2->category
        && e1->vApplClass==e2->vApplClass
        && strcmp(e1->name, e2->name)==0;
}


#define HASH_FUN(elemp) (hashFun(elemp->name) + (unsigned)elemp->vFunClass)
#define HASH_ELEM_EQUAL(e1,e2) equalReferenceItems(e1, e2)

#include "hashlist.tc"


void initReferenceTable(int size) {
    // We want this in cx_memory, so can't use refTabInit() b.c it allocates in StackMemory
    CX_ALLOCC(referenceTable.tab, size, ReferencesItem *);
    refTabNoAllocInit(&referenceTable, size);
}

ReferencesItem *getReferencesItem(int index) {
    assert(index < referenceTable.size);
    assert(referenceTable.tab[index]);
    return referenceTable.tab[index];
}

int getNextExistingReferencesItem(int index) {
    for (int i=index; i < referenceTable.size; i++)
        if (referenceTable.tab[i] != NULL)
            return i;
    return -1;
}

int addToReferencesTable(ReferencesItem *referencesItem) {
    return refTabAdd(&referenceTable, referencesItem);
}

void pushReferences(ReferencesItem *element, int position) {
    refTabPush(&referenceTable, element, position);
}

void setReferencesItem(int index, ReferencesItem *item) {
    referenceTable.tab[index] = item;
}

bool isMemberInReferenceTable(ReferencesItem *element, int *position, ReferencesItem **foundMemberPointer) {
    return refTabIsMember(&referenceTable, element, position, foundMemberPointer);
}

void mapOverReferenceTable(void (*fun)(ReferencesItem *)) {
    refTabMap(&referenceTable, fun);
}

void mapOverReferenceTableWithPointer(void (*fun)(ReferencesItem *, void *), void *pointer) {
    refTabMapWithPointer(&referenceTable, fun, pointer);
}

void mapOverReferenceTableWithIndex(void (*fun)(int index)) {
    refTabMapWithIndex(&referenceTable, fun);
}
