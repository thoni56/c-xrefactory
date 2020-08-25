#include <cgreen/cgreen.h>

#include "options.h"

#include "globals.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "main.mock"
#include "editor.mock"
#include "fileio.mock"


Describe(Options);
BeforeEach(Options) {}
AfterEach(Options) {}


Ensure(Options, will_return_false_if_package_structure_does_not_exist) {
    assert_that(!packageOnCommandLine("org.nonexistant"));
}

Ensure(Options, will_return_true_if_package_structure_exists) {
    javaSourcePaths = ".";
    expect(dirExists, when(fullPath, is_equal_to_string("./org/existant")),
           will_return(true));
    expect(dirInputFile);

    assert_that(packageOnCommandLine("org.existant"));
}

Ensure(Options, will_return_true_if_package_structure_exists_in_search_path) {
    javaSourcePaths = "not/this/path:but/this/path";
    expect(dirExists, when(fullPath, is_equal_to_string("not/this/path/org/existant")),
           will_return(false));
    expect(dirExists, when(fullPath, is_equal_to_string("but/this/path/org/existant")),
           will_return(true));
    expect(dirInputFile);

    assert_that(packageOnCommandLine("org.existant"));
}

Ensure(Options, can_get_java_class_and_source_paths) {
    getJavaClassAndSourcePath();
}


extern int getOptionFromFile(FILE *file, char *text, int *chars_read);

Ensure(Options, can_read_double_quoted_option_string) {
    char option_string_read[MAX_OPTION_LEN];
    int len;

    /* Prepare to read "aaa" */
    expect(readChar, will_return('"'));
    expect(readChar, will_return('a'), times(3));
    expect(readChar, will_return('"'));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(3));
    assert_that(option_string_read, is_equal_to_string("aaa"));
}

Ensure(Options, can_read_backquoted_option_string) {
    char option_string_read[MAX_OPTION_LEN];
    int len;

    /* Prepare to read "aaa" */
    expect(readChar, will_return('`'));
    expect(readChar, will_return('n'), times(2));
    expect(readChar, will_return('l'), times(2));
    expect(readChar, will_return('`'));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(6));
    assert_that(option_string_read, is_equal_to_string("`nnll`"));
}
