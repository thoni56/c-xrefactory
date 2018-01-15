#include <cgreen/cgreen.h>

#include "../filetab.h"

Describe(FileTab);
BeforeEach(FileTab) {}
AfterEach(FileTab) {}

static struct fileTab fileTab;

Ensure(FileTab, can_fetch_a_stored_filename) {
    S_fileItem fileItem = {"file.c"};
    int position = 1;
    int fetched_position;

    fileTabInit(&fileTab, 5);
    fileTabAdd(&fileTab, &fileItem, &position);
    assert_that(fileTabIsMember(&fileTab, &fileItem, &fetched_position));
    assert_that(fetched_position, is_equal_to(position));
}
