#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>
#include <string.h>

#include "commons.h"

#include "filedescriptor.mock"
#include "fileio.mock"
#include "globals.mock"
#include "log.h"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "semact.mock"
#include "yylex.mock"

Describe(Commons);
BeforeEach(Commons) {
    strcpy(cwd, "/home/user/project");
    options.fileNamesCaseSensitive = true;
    log_set_level(LOG_ERROR);
}
AfterEach(Commons) {}

/* normalizeFileName_static tests */

Ensure(Commons, resolves_lone_dot_to_relative_to_directory) {
    char *result = normalizeFileName_static(".", "/home/user/project");
    assert_that(result, is_equal_to_string("/home/user/project"));
}

Ensure(Commons, resolves_lone_dotdot_to_parent_directory) {
    char *result = normalizeFileName_static("..", "/home/user/project");
    assert_that(result, is_equal_to_string("/home/user"));
}

/* extractPathInto tests */

Ensure(Commons, will_extract_nothing_if_no_path) {
    char *source = "nopath";
    char  dest[100];
    int   length;

    length = extractPathInto(source, dest);
    assert_that(strlen(dest) == 0); /* No path to copy */
    assert_that(length, is_equal_to(0));
}

Ensure(Commons, will_extract_only_path_if_there_is_one) {
    char *source = "path/file";
    char  dest[100];
    int   length;

    length = extractPathInto(source, dest);
    assert_that(dest, is_equal_to_string("path/")); /* Copy only path */
    assert_that(length, is_equal_to(strlen("path/")));
}
