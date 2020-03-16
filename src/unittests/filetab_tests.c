#include <cgreen/cgreen.h>

#include "../filetab.h"

#include "misc.mock"
#include "commons.mock"

Describe(FileTab);
BeforeEach(FileTab) {}
AfterEach(FileTab) {}

static struct fileTab fileTab;

Ensure(FileTab, can_fetch_a_stored_filename) {
    S_fileItem fileItem = {"file.c"};
    int position_1 = 1;
    int position_3 = 3;
    int fetched_position;

    fileTabInit(&fileTab, 5);
    position_1 = fileTabAdd(&fileTab, &fileItem);
    position_3 = fileTabAdd(&fileTab, &fileItem);

    assert_that(fileTabIsMember(&fileTab, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position_1));
}

Ensure(FileTab, cannot_find_filename_not_in_tab) {
    S_fileItem exists = {"exists.c"};
    S_fileItem donotexist = {"donot_exist.c"};
    int out_position = -1;

    fileTabInit(&fileTab, 5);
    out_position = fileTabAdd(&fileTab, &exists);

    assert_that(fileTabIsMember(&fileTab, &exists, &out_position));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &out_position));
}

Ensure(FileTab, can_add_and_find_multiple_files) {
    S_fileItem exists1 = {"exists1.c"};
    S_fileItem exists2 = {"exists2.c"};
    S_fileItem exists3 = {"exists3.c"};
    S_fileItem donotexist = {"donot_exist.c"};
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
    S_fileItem exists1 = {"exists1.c"};
    S_fileItem exists2 = {"exists2.c"};
    S_fileItem exists3 = {"exists3.c"};
    S_fileItem donotexist = {"donot_exist.c"};
    int out_position = -1;

    fileTabInit(&fileTab, 6);
    out_position = fileTabAdd(&fileTab, &exists1);
    out_position = fileTabAdd(&fileTab, &exists2);
    out_position = fileTabAdd(&fileTab, &exists3);

    assert_that(!fileTabExists(&fileTab, "anything"));
    assert_that(!fileTabExists(&fileTab, "donot_exist.c"));
    assert_that(fileTabExists(&fileTab, "exists1.c"));
    assert_that(fileTabExists(&fileTab, "exists2.c"));
    assert_that(fileTabExists(&fileTab, "exists3.c"));
}
