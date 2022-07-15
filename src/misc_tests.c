#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "misc.h"

#include "memory.h"

#include "caching.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "options.mock"
#include "ppc.mock"
#include "yylex.mock"

Describe(Misc);
BeforeEach(Misc) {
    log_set_level(LOG_ERROR);
}
AfterEach(Misc) {}

/* Protected */
extern char *concatDirectoryWithFileName(char *result, char *directoryName, char *packageFilename);

Ensure(Misc, can_concat_filename_with_directory) {
    char *dirname  = "dirname";
    char *filename = "filename";
    char  buffer[1000];
    char *result = concatDirectoryWithFileName(buffer, dirname, filename);

    /* TODO: FILE_PATH_SEPARATOR might be '\\' */
    assert_that(result, is_equal_to_string("dirname/filename"));
}

Ensure(Misc, can_see_if_string_contains_wildcard) {
    assert_that(!containsWildcard(""));
    assert_that(!containsWildcard("sldlsadkj 234 %¤!#¤!¤"));
    assert_that(containsWildcard("?"));
    assert_that(containsWildcard("*"));
    assert_that(containsWildcard("[abc]"));
    assert_that(containsWildcard("abc?"));
    assert_that(containsWildcard("abc*"));
    assert_that(containsWildcard("abc[abc]"));
}

static char       *my_a1 = "a1";
static char       *my_a2 = "a2";
static Completions completions;
static void       *my_a4 = &my_a4;
static int         i     = 0;

static void mapFunction(MAP_FUN_SIGNATURE) {
    assert_that(a1, is_equal_to(my_a1));
    assert_that(a2, is_equal_to(my_a2));
    assert_that(a3, is_equal_to(&completions));
    assert_that(a4, is_equal_to(my_a4));
    (*a5)++;
}

Ensure(Misc, can_map_over_directory_files) {
    expect(isDirectory, will_return(true));
    mapDirectoryFiles("/", mapFunction, true, my_a1, my_a2, &completions, my_a4, &i);
    assert_that(i, is_greater_than(0));
}

Ensure(Misc, strmcpy_returns_pointer_to_after_copy) {
    char src[100] = "hello world!";
    char dest[100];

    assert_that(strmcpy(dest, src), is_equal_to(dest+strlen(src)));
}

Ensure(Misc, strmcpy_can_copy_overlapping_strings) {
    char src[100] = "hello world!";
    char *dest = src;

    strmcpy(dest, &src[1]);
    assert_that(src, is_equal_to_string("ello world!"));
}
