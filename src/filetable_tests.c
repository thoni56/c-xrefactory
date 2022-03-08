#include <cgreen/cgreen.h>

#include "filetable.h"
#include "log.h"

#include "misc.mock"
#include "commons.mock"
#include "globals.mock"
#include "cxref.mock"
#include "caching.mock"


Describe(FileTable);
BeforeEach(FileTable) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();
    fileTableInit(&fileTable, 5);
}
AfterEach(FileTable) {}


Ensure(FileTable, can_fetch_a_stored_filename) {
    FileItem fileItem = {"file.c"};
    int position_1 = 1;
    int fetched_position;

    position_1 = fileTableAdd(&fileTable, &fileItem);
    fileTableAdd(&fileTable, &fileItem);

    assert_that(fileTableIsMember(&fileTable, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position_1));
}

Ensure(FileTable, cannot_find_filename_not_in_tab) {
    FileItem exists = {"exists.c"};
    FileItem donotexist = {"donot_exist.c"};
    int index = -1;

    index = fileTableAdd(&fileTable, &exists);

    assert_that(fileTableIsMember(&fileTable, &exists, &index));
    assert_that(!fileTableIsMember(&fileTable, &donotexist, &index));
}

Ensure(FileTable, can_add_and_find_multiple_files) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};
    FileItem donotexist = {"donot_exist.c"};
    int index = -1;

    index = fileTableAdd(&fileTable, &exists1);

    assert_that(fileTableIsMember(&fileTable, &exists1, &index));
    assert_that(!fileTableIsMember(&fileTable, &donotexist, &index));

    index = fileTableAdd(&fileTable, &exists2);
    assert_that(fileTableIsMember(&fileTable, &exists1, &index));
    assert_that(fileTableIsMember(&fileTable, &exists2, &index));
    assert_that(!fileTableIsMember(&fileTable, &donotexist, &index));

    index = fileTableAdd(&fileTable, &exists3);
    assert_that(fileTableIsMember(&fileTable, &exists1, &index));
    assert_that(fileTableIsMember(&fileTable, &exists2, &index));
    assert_that(fileTableIsMember(&fileTable, &exists3, &index));
    assert_that(!fileTableIsMember(&fileTable, &donotexist, &index));
}

Ensure(FileTable, can_check_filename_exists) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};

    fileTableAdd(&fileTable, &exists1);
    fileTableAdd(&fileTable, &exists2);
    fileTableAdd(&fileTable, &exists3);

    assert_that(!fileExistsInTable(&fileTable, "anything"));
    assert_that(!fileExistsInTable(&fileTable, "donot_exist.c"));
    assert_that(fileExistsInTable(&fileTable, "exists1.c"));
    assert_that(fileExistsInTable(&fileTable, "exists2.c"));
    assert_that(fileExistsInTable(&fileTable, "exists3.c"));
}

Ensure(FileTable, can_lookup_filename) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};
    int index = -1;

    assert_that(fileTableLookup(&fileTable, "donot_exist.c"), is_equal_to(-1));

    index = fileTableAdd(&fileTable, &exists1);
    assert_that(fileTableLookup(&fileTable, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTableLookup(&fileTable, "exists1.c"), is_equal_to(index));

    index = fileTableAdd(&fileTable, &exists2);
    assert_that(fileTableLookup(&fileTable, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTableLookup(&fileTable, "exists2.c"), is_equal_to(index));

    index = fileTableAdd(&fileTable, &exists3);
    assert_that(fileTableLookup(&fileTable, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTableLookup(&fileTable, "exists3.c"), is_equal_to(index));
}

Ensure(FileTable, can_get_fileitem) {
    FileItem item = {"item.c"};
    int index = fileTableAdd(&fileTable, &item);

    FileItem *gotten = getFileItem(index);

    /* Has the same name */
    assert_that(gotten->name, is_equal_to_string(item.name));

    /* And is actually the same item... */
    assert_that(gotten, is_equal_to(&item));
}
