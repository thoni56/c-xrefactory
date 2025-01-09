#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "misc.h"

#include "caching.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "options.mock"
#include "ppc.mock"
#include "proto.h"
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
    assert_that(!containsWildcard("sldlsadkj 234 %%!#%!%"));
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
    mapOverDirectoryFiles("/", mapFunction, true, my_a1, my_a2, &completions, my_a4, &i);
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

Ensure(Misc, pathncmp_can_compare_paths) {
    assert_that(pathncmp("abc", "abc", 3, true), is_equal_to(0));
    assert_that(pathncmp("abcd", "abc", 3, true), is_equal_to(0));
    assert_that(pathncmp("abc", "abcd", 3, true), is_equal_to(0));

    assert_that(pathncmp("AbC", "abc", 3, true), is_not_equal_to(0));
    assert_that(pathncmp("AbC", "abc", 3, false), is_equal_to(0));
    assert_that(pathncmp("abcd", "AbC", 3, true), is_not_equal_to(0));
    assert_that(pathncmp("abcd", "AbC", 3, false), is_equal_to(0));
    assert_that(pathncmp("AbC", "aBcd", 3, true), is_not_equal_to(0));
    assert_that(pathncmp("AbC", "aBcd", 3, false), is_equal_to(0));

    assert_that(pathncmp("AbC", "aBcd", 4, false), is_not_equal_to(0));

    //assert_that(pathncmp("path/abc", "path\abc", 8, true), is_equal_to(0));
}

Ensure(Misc, will_map_over_paths_for_each_entry_setting_currentPath) {
    char *paths = "a:b:c";      /* TODO Windows path separator */
    int count = 0;
    char mappedPaths[3][2];

    MAP_OVER_PATHS(paths, {
            strcpy(mappedPaths[count++], currentPath);
        });

    assert_that(count, is_equal_to(3));
    assert_that(mappedPaths[0], is_equal_to_string("a"));
    assert_that(mappedPaths[1], is_equal_to_string("b"));
    assert_that(mappedPaths[2], is_equal_to_string("c"));
}

Ensure(Misc, can_prettyprint_simple_linkname) {
    char pretty_printed[100];
    prettyPrintLinkName(pretty_printed, "a_name", sizeof(pretty_printed), LONG_NAME);

    assert_that(pretty_printed, is_equal_to_string("a_name"));
}

Ensure(Misc, can_prettyprint_linkname_with_file_reference) {
    char pretty_printed[100];
    prettyPrintLinkName(pretty_printed, "some_file!a_name", sizeof(pretty_printed), LONG_NAME);

    assert_that(pretty_printed, is_equal_to_string("a_name"));
}
