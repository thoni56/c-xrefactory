#include <cgreen/cgreen.h>

#include "../filetab.h"

#include "misc.mock"
#include "commons.mock"
#include "memory.mock"


Describe(FileTab);
BeforeEach(FileTab) {}
AfterEach(FileTab) {}

static struct fileTab fileTab;

Ensure(FileTab, can_fetch_a_stored_filename) {
    FileItem fileItem = {"file.c"};
    int position_1 = 1;
    int fetched_position;

    fileTabInit(&fileTab, 5);
    position_1 = fileTabAdd(&fileTab, &fileItem);
    fileTabAdd(&fileTab, &fileItem);

    assert_that(fileTabIsMember(&fileTab, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position_1));
}

Ensure(FileTab, cannot_find_filename_not_in_tab) {
    FileItem exists = {"exists.c"};
    FileItem donotexist = {"donot_exist.c"};
    int out_position = -1;

    fileTabInit(&fileTab, 5);
    out_position = fileTabAdd(&fileTab, &exists);

    assert_that(fileTabIsMember(&fileTab, &exists, &out_position));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &out_position));
}

Ensure(FileTab, can_add_and_find_multiple_files) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};
    FileItem donotexist = {"donot_exist.c"};
    int out_position = -1;

    fileTabInit(&fileTab, 5);
    out_position = fileTabAdd(&fileTab, &exists1);

    assert_that(fileTabIsMember(&fileTab, &exists1, &out_position));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &out_position));

    out_position = fileTabAdd(&fileTab, &exists2);
    assert_that(fileTabIsMember(&fileTab, &exists1, &out_position));
    assert_that(fileTabIsMember(&fileTab, &exists2, &out_position));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &out_position));

    out_position = fileTabAdd(&fileTab, &exists3);
    assert_that(fileTabIsMember(&fileTab, &exists1, &out_position));
    assert_that(fileTabIsMember(&fileTab, &exists2, &out_position));
    assert_that(fileTabIsMember(&fileTab, &exists3, &out_position));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &out_position));
}

Ensure(FileTab, can_check_filename_exists) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};

    fileTabInit(&fileTab, 6);
    fileTabAdd(&fileTab, &exists1);
    fileTabAdd(&fileTab, &exists2);
    fileTabAdd(&fileTab, &exists3);

    assert_that(!fileTabExists(&fileTab, "anything"));
    assert_that(!fileTabExists(&fileTab, "donot_exist.c"));
    assert_that(fileTabExists(&fileTab, "exists1.c"));
    assert_that(fileTabExists(&fileTab, "exists2.c"));
    assert_that(fileTabExists(&fileTab, "exists3.c"));
}

Ensure(FileTab, can_lookup_filename) {
    FileItem exists1 = {"exists1.c"};
    FileItem exists2 = {"exists2.c"};
    FileItem exists3 = {"exists3.c"};
    int position = -1;

    fileTabInit(&fileTab, 6);
    assert_that(fileTabLookup(&fileTab, "donot_exist.c"), is_equal_to(-1));

    position = fileTabAdd(&fileTab, &exists1);
    assert_that(fileTabLookup(&fileTab, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTabLookup(&fileTab, "exists1.c"), is_equal_to(position));

    position = fileTabAdd(&fileTab, &exists2);
    assert_that(fileTabLookup(&fileTab, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTabLookup(&fileTab, "exists2.c"), is_equal_to(position));

    position = fileTabAdd(&fileTab, &exists3);
    assert_that(fileTabLookup(&fileTab, "donot_exist.c"), is_equal_to(-1));
    assert_that(fileTabLookup(&fileTab, "exists3.c"), is_equal_to(position));
}
