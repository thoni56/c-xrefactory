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

    expect(hashFun, will_return(5));
    expect(hashFun, will_return(6));
    expect(hashFun, will_return(5)); /* Same as first hash */

    fileTabInit(&fileTab, 5);
    fileTabAdd(&fileTab, &fileItem, &position_1);
    fileTabAdd(&fileTab, &fileItem, &position_3);

    assert_that(fileTabIsMember(&fileTab, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position_1));
}

Ensure(FileTab, cannot_find_filename_not_in_tab) {
    S_fileItem fileItem1 = {"exists.c"};
    S_fileItem fileItem2 = {"donnot_exist.c"};
    int position_1 = 1;

    expect(hashFun, will_return(5));
    expect(hashFun, will_return(6));

    fileTabInit(&fileTab, 5);
    fileTabAdd(&fileTab, &fileItem1, &position_1);

    assert_that(!fileTabIsMember(&fileTab, &fileItem2, &position_1));
}
