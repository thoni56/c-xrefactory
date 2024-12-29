#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "filetable.h"
#include "log.h"

#include "caching.mock"
#include "commons.mock"
#include "cxref.mock"
#include "globals.mock"
#include "misc.mock"
#include "stackmemory.h"


Describe(FileTable);
BeforeEach(FileTable) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();
    initFileTable(100);
}
AfterEach(FileTable) {}

Ensure(FileTable, can_check_multiple_filenames_exists) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};

    addToFileTable(&exists1);
    addToFileTable(&exists2);
    addToFileTable(&exists3);

    assert_that(!existsInFileTable("anything"));
    assert_that(!existsInFileTable("donot_exist.c"));
    assert_that(existsInFileTable("exists1.c"));
    assert_that(existsInFileTable("exists2.c"));
    assert_that(existsInFileTable("exists3.c"));
}

Ensure(FileTable, can_lookup_filename) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};
    int      index   = -1;

    assert_that(getFileNumberFromFileName("donot_exist.c"), is_equal_to(-1));

    index = addToFileTable(&exists1);
    assert_that(getFileNumberFromFileName("donot_exist.c"), is_equal_to(-1));
    assert_that(getFileNumberFromFileName("exists1.c"), is_equal_to(index));

    index = addToFileTable(&exists2);
    assert_that(getFileNumberFromFileName("donot_exist.c"), is_equal_to(-1));
    assert_that(getFileNumberFromFileName("exists2.c"), is_equal_to(index));

    index = addToFileTable(&exists3);
    assert_that(getFileNumberFromFileName("donot_exist.c"), is_equal_to(-1));
    assert_that(getFileNumberFromFileName("exists3.c"), is_equal_to(index));
}

Ensure(FileTable, can_get_fileitem) {
    FileItem item  = {"item.c"};
    int      index = addToFileTable(&item);

    FileItem *gotten = getFileItem(index);

    /* Has the same name */
    assert_that(gotten->name, is_equal_to_string(item.name));

    /* And is actually the same item... */
    assert_that(gotten, is_equal_to(&item));
}

Ensure(FileTable, can_return_next_existing_file_index) {
    FileItem item  = {"item.c"};
    int      index = addToFileTable(&item);

    assert_that(getNextExistingFileNumber(0), is_equal_to(index));
}

Ensure(FileTable, will_return_error_for_no_more_next_file_item) {
    assert_that(getNextExistingFileNumber(0), is_equal_to(-1));
}

static int  mapFunctionCalled = 0;
static void mapFunction(FileItem *fileItem) {
    mapFunctionCalled++;
}

Ensure(FileTable, can_map) {
    FileItem item1 = {"item1.c"};
    FileItem item2 = {"item2.c"};
    addToFileTable(&item1);
    addToFileTable(&item2);

    mapFunctionCalled = 0;

    mapOverFileTable(mapFunction);

    assert_that(mapFunctionCalled, is_equal_to(2));
}

static int  mapFunctionWithIndexCalled = 0;
static void mapFunctionWithIndex(FileItem *fileItem, int index) {
    mapFunctionWithIndexCalled++;
}

Ensure(FileTable, can_map_with_index) {
    FileItem item = {"item.c"};
    addToFileTable(&item);

    mapFunctionWithIndexCalled = 0;

    mapOverFileTableWithIndex(mapFunctionWithIndex);

    assert_that(mapFunctionWithIndexCalled, is_equal_to(1));
}

static int variable;

static int  mapFunctionWithPointerCalled = 0;
static void mapFunctionWithPointer(FileItem *fileItem, void *pointer) {
    assert_that(pointer, is_equal_to(&variable));
    mapFunctionWithPointerCalled++;
}

Ensure(FileTable, can_map_with_pointer) {
    FileItem item1 = {"item1.c"};
    FileItem item2 = {"item2.c"};
    FileItem item3 = {"item3.c"};
    addToFileTable(&item1);
    addToFileTable(&item2);
    addToFileTable(&item3);

    mapFunctionCalled = 0;

    mapOverFileTableWithPointer(mapFunctionWithPointer, &variable);

    assert_that(mapFunctionWithPointerCalled, is_equal_to(3));
}
