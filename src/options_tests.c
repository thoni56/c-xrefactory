#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "filetable.h"
#include "options.h"
#include "log.h"

#include "globals.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "main.mock"
#include "editor.mock"
#include "filetable.mock"
#include "fileio.mock"
#include "yylex.mock"
#include "ppc.mock"

static bool optionsOverflowHandler() {
    return false;
}


Describe(Options);
BeforeEach(Options) {
    log_set_level(LOG_ERROR);
    initMemory(&options.memory, optionsOverflowHandler, SIZE_opiMemory);
}
AfterEach(Options) {}


Ensure(Options, will_return_false_if_package_structure_does_not_exist) {
    assert_that(!packageOnCommandLine("org.nonexistant"));
}

Ensure(Options, will_return_true_if_package_structure_exists) {
    javaSourcePaths = ".";
    expect(directoryExists, when(fullPath, is_equal_to_string("./org/existant")),
           will_return(true));
    expect(normalizeFileName, when(name, is_equal_to_string("./org/existant")), will_return("./org/existant"));
    expect(getFileSuffix, will_return(""));
    expect(editorFileStatus, will_return(0));

    expect(addFileTableItem, will_return(42));
    FileItem fileItem = {"./org/existant"};
    expect(getFileItem, when(fileIndex, is_equal_to(42)),
           will_return(&fileItem));

    assert_that(packageOnCommandLine("org.existant"));
}

Ensure(Options, will_return_true_if_package_structure_exists_in_search_path) {
    javaSourcePaths = "not/this/path:but/this/path";
    expect(directoryExists, when(fullPath, is_equal_to_string("not/this/path/org/existant")),
           will_return(false));
    expect(directoryExists, when(fullPath, is_equal_to_string("but/this/path/org/existant")),
           will_return(true));
    expect(normalizeFileName, when(name, is_equal_to_string("but/this/path/org/existant")),
           will_return("but/this/path/org/existant"));
    expect(getFileSuffix, will_return(""));
    expect(editorFileStatus);

    expect(addFileTableItem, will_return(42));
    FileItem fileItem = {"but/this/path/org/existant"};
    expect(getFileItem, when(fileIndex, is_equal_to(42)),
           will_return(&fileItem));

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

Ensure(Options, will_not_expand_special_file_variable_when_no_value) {
    expect(getRealFileName_static, when(fn, is_equal_to_string("options.c")),
           will_return("/some/path/to/options.c"));
    expect(directoryName_st, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("/some/path/to"));
    expect(simpleFileNameWithoutSuffix_st, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("options"));
    expect(lastOccurenceInString, when(string, is_equal_to_string("/some/path/to/options.c")),
           will_return(".c"));
    expect(javaDotifyFileName, times(2));

    char *expanded = expandSpecialFilePredefinedVariables_st("cp", "options.c");
    assert_that(expanded, is_equal_to_string("cp"));
}

Ensure(Options, can_expand_special_variable_file) {
    expect(getRealFileName_static, when(fn, is_equal_to_string("options.c")),
           will_return("/some/path/to/options.c"));
    expect(directoryName_st, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("/some/path/to"));
    expect(simpleFileNameWithoutSuffix_st, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("options"));
    expect(lastOccurenceInString, when(string, is_equal_to_string("/some/path/to/options.c")),
           will_return(".c"));
    expect(javaDotifyFileName, times(2));

    char *expanded = expandSpecialFilePredefinedVariables_st("${__file}", "options.c");
    assert_that(expanded, is_equal_to_string("/some/path/to/options.c"));
}

Ensure(Options, can_allocate_a_string) {
    char *allocatedString;

    assert_that(options.memory.index, is_equal_to(0));
    createOptionString(&allocatedString, "allocated string");
    assert_that(allocatedString, is_equal_to_string("allocated string"));
    assert_that(options.memory.index, is_greater_than(0));
}
