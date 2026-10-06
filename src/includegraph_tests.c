#include <cgreen/cgreen.h>

#include "includegraph.h"

#include "head.h"
#include "log.h"
#include "memory.h"
#include "referenceableitemtable.h"

#include "commons.mock"
#include "filetable.mock"


Describe(IncludeGraph);
BeforeEach(IncludeGraph) {
    log_set_level(LOG_ERROR);
    initCxMemory(10000);
    initReferenceableItemTable(100);
}
AfterEach(IncludeGraph) {}


#define A_C 1
#define A_H 2
#define DECL_H 3
#define B_H 4

/* As the parser records it: one item per included file, with a reference
 * positioned in each file that includes it. */
static void recordIncludeReference(int includerFileNumber, int includedFileNumber, Usage usage) {
    ReferenceableItem searchItem = makeReferenceableItem(LINK_NAME_INCLUDE_REFS,
                                                         TypeCppInclude,
                                                         StorageExtern, VisibilityGlobal,
                                                         includedFileNumber);
    ReferenceableItem *item;
    if (!isMemberInReferenceableItemTable(&searchItem, NULL, &item)) {
        item = cxAlloc(sizeof(ReferenceableItem));
        *item = searchItem;
        addToReferenceableItemTable(item);
    }
    Reference *reference = cxAlloc(sizeof(Reference));
    *reference = (Reference){.position = makePosition(includerFileNumber, 1, 0),
        .usage = usage, .next = item->references};
    item->references = reference;
}

static void recordInclude(int includerFileNumber, int includedFileNumber) {
    recordIncludeReference(includerFileNumber, includedFileNumber, UsageUsed);
}

/* As addFileAsIncludeReference() records it: a "defined" reference at line 1
 * of the file itself, marking the file, not an include. */
static void recordFileItself(int fileNumber) {
    recordIncludeReference(fileNumber, fileNumber, UsageDefined);
}


Ensure(IncludeGraph, gives_the_file_a_compilation_unit_includes) {
    recordInclude(A_C, A_H);

    int closure[10];
    int count = collectIncludeClosure(A_C, closure, 10);

    assert_that(count, is_equal_to(1));
    assert_that(closure[0], is_equal_to(A_H));
}


static void recordSymbolReference(char *linkName, int fileNumber) {
    ReferenceableItem *item = cxAlloc(sizeof(ReferenceableItem));
    *item = makeReferenceableItem(linkName, TypeFunction, StorageExtern, VisibilityGlobal,
                                  NO_FILE_NUMBER);
    Reference *reference = cxAlloc(sizeof(Reference));
    *reference = (Reference){.position = makePosition(fileNumber, 1, 0), .usage =
        UsageUsed};
    item->references = reference;
    addToReferenceableItemTable(item);
}

Ensure(IncludeGraph, does_not_collect_other_symbols_for_includes) {
    recordSymbolReference("f", A_C);

    int closure[10];
    int count = collectIncludeClosure(A_C, closure, 10);

    assert_that(count, is_equal_to(0));
}


static bool contains(int fileNumbers[], int count, int fileNumber) {
    for (int i = 0; i < count; i++)
        if (fileNumbers[i] == fileNumber)
            return true;
    return false;
}

Ensure(IncludeGraph, collects_also_the_files_an_included_file_includes) {
    recordInclude(A_C, A_H);
    recordInclude(A_H, DECL_H);

    int closure[10];
    int count = collectIncludeClosure(A_C, closure, 10);

    assert_that(count, is_equal_to(2));
    assert_true(contains(closure, count, A_H));
    assert_true(contains(closure, count, DECL_H));
}

Ensure(IncludeGraph, collects_a_file_included_twice_once) {
    recordInclude(A_C, A_H);
    recordInclude(A_C, DECL_H);
    recordInclude(A_H, DECL_H);

    int closure[10];
    int count = collectIncludeClosure(A_C, closure, 10);

    assert_that(count, is_equal_to(2));
}

Ensure(IncludeGraph, does_not_collect_the_file_itself) {
    recordFileItself(A_C);
    recordInclude(A_C, A_H);

    int closure[10];
    int count = collectIncludeClosure(A_C, closure, 10);

    assert_that(count, is_equal_to(1));
    assert_that(closure[0], is_equal_to(A_H));
}

Ensure(IncludeGraph, collecting_can_signal_that_the_files_do_not_fit) {
    recordInclude(A_C, A_H);
    recordInclude(A_C, B_H);
    recordInclude(A_C, DECL_H);

    int closure[3] = {-1, -1, -1};
    int count = collectIncludeClosure(A_C, closure, 2);

    assert_that(count, is_greater_than(2));
    assert_that(closure[2], is_equal_to(-1));
}
