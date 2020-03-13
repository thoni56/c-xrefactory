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
    fileTabAdd(&fileTab, &fileItem, &position_1);
    fileTabAdd(&fileTab, &fileItem, &position_3);

    assert_that(fileTabIsMember(&fileTab, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position_1));
}

Ensure(FileTab, cannot_find_filename_not_in_tab) {
    S_fileItem exists = {"exists.c"};
    S_fileItem donotexist = {"donot_exist.c"};
    int position_1 = -1;

    fileTabInit(&fileTab, 5);
    fileTabAdd(&fileTab, &exists, &position_1);

    assert_that(fileTabIsMember(&fileTab, &exists, &position_1));
    assert_that(!fileTabIsMember(&fileTab, &donotexist, &position_1));
}
