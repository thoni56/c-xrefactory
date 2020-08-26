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

Ensure(Options, can_read_a_normal_option_string) {
    char option_string[] = "this is a double quoted option";
    char expected_option_value[] = "this";
    char option_string_read[MAX_OPTION_LEN];
    int len;

    for (int i = 0; i<=4; i++)
        expect(readChar, will_return(option_string[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_double_quoted_option_string) {
    char option_string[] = "\"this is a double quoted option\"";
    char expected_option_value[] = "this is a double quoted option";
    char option_string_read[MAX_OPTION_LEN];
    int len;

    for (int i = 0; i<=strlen(option_string)-1; i++)
        expect(readChar, will_return(option_string[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_backquoted_option_string) {
    char expected_option_value[] = "`this is a backtick quoted option`";
    char option_string_read[MAX_OPTION_LEN];
    int len;

    for (int i = 0; i<=strlen(expected_option_value)-1; i++)
        expect(readChar, will_return(expected_option_value[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_square_bracketed_option_strings) {
    char expected_option_value[] = "[this is an option]";
    char option_string_read[MAX_OPTION_LEN];
    int len;

    for (int i = 0; i<=strlen(expected_option_value)-1; i++)
        expect(readChar, will_return(expected_option_value[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}
