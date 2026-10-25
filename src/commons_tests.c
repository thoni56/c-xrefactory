#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>
#include <string.h>

#include "commons.h"
#include "protocol.h"

#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "log.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
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

/* initCwd tests */

Ensure(Commons, reports_and_resolves_against_the_root_when_the_working_directory_is_unknown) {
    options.cxrefProtocol = true;
    expect(getCwd, will_return(NULL));
    expect(ppcGenRecord, when(kind, is_equal_to_string(PPC_ERROR)));

    initCwd();

    assert_that(cwd, is_equal_to_string("/"));
    options.cxrefProtocol = false;
}

Ensure(Commons, stays_silenced_until_every_silence_is_resumed) {
    options.cxrefProtocol = true;
    options.errors = true;
    silenceMessages();
    silenceMessages();
    resumeMessages();

    never_expect(ppcGenRecord);
    announceAnalysing("source.c");

    resumeMessages();
    options.cxrefProtocol = false;
    options.errors = false;
}
