#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "argumentsvector.h"
#include "constants.h"

#include "globals.h"
#include "log.h"
#include "memory.h"
#include "options.h"

#include "commandlogger.mock"
#include "commons.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "proto.h"
#include "yylex.mock"


Describe(Options);
BeforeEach(Options) {
    log_set_level(LOG_ERROR);
    memoryInit(&options.memory, "", NULL, OptionsMemorySize);
}
AfterEach(Options) {}


extern int getOptionFromFile(FILE *file, char *text, int *chars_read);

Ensure(Options, will_return_eof_when_end_of_file_and_nothing_read) {
    FILE *someFile = NULL;
    char option_string_read[MAX_OPTION_LEN];
    int chars_read;

    expect(readChar, will_return(EOF));

    assert_that(getOptionFromFile(someFile, option_string_read, &chars_read), is_equal_to(EOF));
    assert_that(strlen(option_string_read), is_equal_to(0));
    assert_that(chars_read, is_equal_to(0));
}

Ensure(Options, can_read_a_normal_option_string_delimited_by_a_space) {
    char option_string[]         = "this <- is delimited by a space";
    char expected_option_value[] = "this";
    char option_string_read[MAX_OPTION_LEN];
    int  length;

    for (int i = 0; i <= 4; i++)
        expect(readChar, will_return(option_string[i]));

    assert_that(getOptionFromFile(NULL, option_string_read, &length), is_not_equal_to(EOF));
    assert_that(length, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_double_quoted_option_string) {
    char option_string[]         = "\"this is a double quoted option\"";
    char expected_option_value[] = "this is a double quoted option";
    char option_string_read[MAX_OPTION_LEN];
    int  len;

    for (int i = 0; i <= strlen(option_string) - 1; i++)
        expect(readChar, will_return(option_string[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_backquoted_option_string) {
    char expected_option_value[] = "`this is a backtick quoted option`";
    char option_string_read[MAX_OPTION_LEN];
    int  len;

    for (int i = 0; i <= strlen(expected_option_value) - 1; i++)
        expect(readChar, will_return(expected_option_value[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, can_read_square_bracketed_option_strings) {
    char expected_option_value[] = "[this is an option]";
    char option_string_read[MAX_OPTION_LEN];
    int  len;

    for (int i = 0; i <= strlen(expected_option_value) - 1; i++)
        expect(readChar, will_return(expected_option_value[i]));

    getOptionFromFile(NULL, option_string_read, &len);

    assert_that(len, is_equal_to(strlen(expected_option_value)));
    assert_that(option_string_read, is_equal_to_string(expected_option_value));
}

Ensure(Options, will_not_expand_special_file_variable_when_no_value) {
    expect(getRealFileName_static, when(fn, is_equal_to_string("options.c")),
           will_return("/some/path/to/options.c"));
    expect(directoryName_static, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("/some/path/to"));
    expect(simpleFileNameWithoutSuffix_static, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("options"));
    expect(lastOccurenceInString, when(string, is_equal_to_string("/some/path/to/options.c")), will_return(".c"));

    char *expanded = expandPredefinedSpecialVariables_static("cp", "options.c");
    assert_that(expanded, is_equal_to_string("cp"));
}

Ensure(Options, can_expand_special_variable_file) {
    expect(getRealFileName_static, when(fn, is_equal_to_string("options.c")),
           will_return("/some/path/to/options.c"));
    expect(directoryName_static, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("/some/path/to"));
    expect(simpleFileNameWithoutSuffix_static, when(fullFileName, is_equal_to_string("/some/path/to/options.c")),
           will_return("options"));
    expect(lastOccurenceInString, when(string, is_equal_to_string("/some/path/to/options.c")), will_return(".c"));

    char *expanded = expandPredefinedSpecialVariables_static("${__file}", "options.c");
    assert_that(expanded, is_equal_to_string("/some/path/to/options.c"));
}

Ensure(Options, can_allocate_a_string) {
    char *allocatedString;

    assert_that(options.memory.index, is_equal_to(0));
    allocatedString = allocateStringForOption(&allocatedString, "allocated string");
    assert_that(allocatedString, is_equal_to_string("allocated string"));
    assert_that(options.memory.index, is_greater_than(0));
}

// Some random options parsing tests...

Ensure(Options, can_parse_about_command_line_option) {
    char *argv[] = {"", "-about"};
    ArgumentsVector args = {2, argv};

    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
    assert_that(options.serverOperation, is_equal_to(OLO_ABOUT));
}

Ensure(Options, can_parse_version_command_line_option) {
    char *argv[] = {"", "-version"};
    ArgumentsVector args = {2, argv};

    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
    assert_that(options.serverOperation, is_equal_to(OLO_ABOUT));
}

Ensure(Options, can_parse_double_dash_version_command_line_option) {
    char *argv[] = {"", "--version"};
    ArgumentsVector args = {2, argv};

    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
    assert_that(options.serverOperation, is_equal_to(OLO_ABOUT));
}

Ensure(Options, can_parse_xrefrc_option_with_equals) {
    char *argv[] = {"", "-xrefrc=abc"};
    ArgumentsVector args = {2, argv};

    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
    assert_that(options.xrefrc, is_equal_to_string("abc"));
}

Ensure(Options, can_parse_xrefrc_option_with_filename_separate) {
    char *argv[] = {"", "-xrefrc", "abc"};
    ArgumentsVector args = {3, argv};

    processOptions(args, PROCESS_FILE_ARGUMENTS_NO);
    assert_that(options.xrefrc, is_equal_to_string("abc"));
}

Ensure(Options, can_readOptionsFromFileIntoArgs) {
    FILE *file = NULL;
    char project[1000];
    char unused[1000];

    expect(readChar, will_return(EOF));

    ArgumentsVector nargs;
    readOptionsIntoArgs(file, &nargs, &ppmMemory, "", project, unused);

    assert_that(nargs.argc, is_equal_to(1)); /* Which is actually no arguments... */
}

static void expect_characters(char string[], bool eof) {
    int i;

    for (i = 0; string[i] != '\0'; i++) {
        expect(readChar, will_return(string[i]));
    }
    if (eof)
        expect(readChar, will_return(EOF));
}

xEnsure(Options, can_find_project_options_file_from_sourcefile_path) {
    FILE optionsFile;
    char optionsFilename[1000];
    char section[1000] = "/home/project";

    expect(getEnv, when(variable, is_equal_to_string("HOME")),
           will_return("HOME"));
    expect(openFile, will_return(&optionsFile));
    expect_characters("[/home/project]\n", false);
    expect_characters("  /home/project\n", true);
    expect(pathncmp, when(path1, is_equal_to_string("/home/project")),
           will_return(0));
    expect(closeFile);

    searchForProjectOptionsFileAndProjectForFile("/home/project/sourcefile.c", optionsFilename, section);

    assert_that(optionsFilename, is_equal_to_string("HOME/.c-xrefrc"));
    assert_that(section, is_equal_to_string("/home/project"));
}

static bool errorMessageCalled;
static void errorMessageCallback(void *ignored) {
    errorMessageCalled = true;
}
Ensure(Options, should_error_when_environment_storage_is_exhausted) {
    char name[100];
    char value[] = "some value of 20 characters";
    int i = 1;
    void *data_pointer = NULL;

    expect(errorMessage, with_side_effect(errorMessageCallback, data_pointer));

    /* We should probably calculate sizes to verify exactly when it overflows */
    errorMessageCalled = false;
    while (!errorMessageCalled) {
        sprintf(name, "name%d", i++);
        setOptionVariable(name, value);
    }
    assert_that(true);          /* Will probably crash if it doesn't call errorMessage()  */
}

Ensure(Options, should_get_back_stored_option_variable_value) {
    char *name = "name";
    char *value = "value";

    setOptionVariable(name, value);
    assert_that(getOptionVariable(name), is_equal_to_string(value));
}

xEnsure(Options, can_get_options_file_from_filename_using_searchProjectOptionsFileAndSectionForFile) {
    char optionsFilename[100];
    char sectionName[100];
    FILE file;

    expect(getEnv, when(variable, is_equal_to_string("HOME")),
           will_return("HOME"));
    expect(openFile, when(fileName, is_equal_to_string("HOME/.c-xrefrc")),
           will_return(&file));

    expect_characters("[/path]\n", false);
    expect_characters("-set X Y\n", true); /* WTF: a -set is needed for the project section to be found...
                                            see readOptionsFromFileIntoArgs() */

    expect(pathncmp, when(path1, is_equal_to_string("/path")), when(length, is_equal_to(5)),
           will_return(0)); // 0 means equal

    expect(closeFile, when(file, is_equal_to(&file)));

    searchForProjectOptionsFileAndProjectForFile("/path/filename.c", optionsFilename, sectionName);

    assert_that(optionsFilename, is_equal_to_string("HOME/.c-xrefrc"));
    assert_that(sectionName, is_equal_to_string("/path"));
}


extern bool projectCoveringFileInOptionsFile(char *fileName, FILE *optionsFile, /* out */ char *projectName);

Ensure(Options, can_see_if_a_project_doesnt_cover_file) {
    FILE file;
    FILE *optionsFile = &file;
    char projectName[MAX_SOURCE_PATH_SIZE];

    expect(readChar, will_return(EOF));

    assert_that(!projectCoveringFileInOptionsFile("/usr/user/project/file.c", optionsFile, projectName));
}

Ensure(Options, can_see_if_a_project_in_a_configuration_file_with_a_single_project_on_first_line_covers_file) {
    FILE file;
    FILE *optionsFile = &file;
    char projectName[MAX_SOURCE_PATH_SIZE];

    expect_characters("[/usr/user/project", false);
    expect(readChar, will_return(']'));
    expect(pathncmp, will_return(0));

    assert_that(projectCoveringFileInOptionsFile("/usr/user/project/file.c", optionsFile, projectName));
    assert_that(projectName, is_equal_to_string("/usr/user/project"));
}

Ensure(Options, can_see_if_a_project_in_a_configuration_file_with_a_single_project_preceeded_by_whitespace_covers_file) {
    FILE file;
    FILE *optionsFile = &file;
    char projectName[MAX_SOURCE_PATH_SIZE];

    expect_characters("\n        [/usr/user/project", false);
    expect(readChar, will_return(']'));
    expect(pathncmp, will_return(0));

    assert_that(projectCoveringFileInOptionsFile("/usr/user/project/file.c", optionsFile, projectName));
}

Ensure(Options, can_see_if_a_project_in_a_configuration_file_with_other_project_before_covers_file) {
    FILE file;
    FILE *optionsFile = &file;
    char projectName[MAX_SOURCE_PATH_SIZE];

    expect_characters("[/usr/user/otherproject]\n", false);
    expect_characters("  -set X Y\n", false);
    expect_characters("  /usr/user/project\n", false);
    expect_characters("[/usr/user/project", false);
    expect(readChar, will_return(']'));

    expect(pathncmp, will_return(1)); /* Not a match */
    expect(pathncmp, will_return(0)); /* A match */

    assert_that(projectCoveringFileInOptionsFile("/usr/user/project/file.c",
                                                 optionsFile, projectName));
}


Ensure(Options, can_return_project_options_filename_and_section_in_legacy_mode) {
    char optionsFilename[1000];
    char sectionName[1000];
    FILE file;

    // LEGACY mode: explicit -xrefrc, uses path-based section matching
    options.xrefrc = "HOME/.c-xrefrc";

    expect(normalizeFileName_static, will_return("HOME/.c-xrefrc"));
    expect(openFile, when(fileName, is_equal_to_string("HOME/.c-xrefrc")),
           will_return(&file));

    expect_characters("[ffmpeg]\n", false);
    expect_characters(" /home/thoni/Utveckling/c-xrefactory/tests/ffmpeg/ffmpeg\n", false);
    expect_characters(" -I/home/thoni/Utveckling/c-xrefactory/tests/ffmpeg/ffmpeg\n", true);

    expect(closeFile);

    options.project = "ffmpeg";
    searchForProjectOptionsFileAndProjectForFile("/home/thoni/Utveckling/c-xrefactory/tests/ffmpeg",
                                                 optionsFilename, sectionName);

    assert_that(sectionName, is_equal_to_string("ffmpeg"));
}

Ensure(Options, can_return_project_name_from_autodetected_config) {
    char optionsFilename[1000];
    char sectionName[1000];
    FILE file;

    // AUTO-DETECT mode: no explicit -xrefrc, searches upward, only reads section header
    always_expect(getEnv, will_return("HOME"));
    expect(isDirectory, will_return(false));
    expect(directoryName_static, will_return("HOME/projectdir"));
    expect(fileExists, will_return(true)); /* HOME/projectdir/.c-xrefrc */

    expect(openFile, when(fileName, is_equal_to_string("HOME/projectdir/.c-xrefrc")),
           will_return(&file));

    /* getProjectNameFromOptionsFile only reads up to the ']' */
    expect_characters("[myproject", false);
    expect(readChar, will_return(']'));

    expect(closeFile);

    searchForProjectOptionsFileAndProjectForFile("/home/thoni/Utveckling/c-xrefactory/tests/myproject",
                                               optionsFilename, sectionName);

    assert_that(sectionName, is_equal_to_string("myproject"));
}

Ensure(Options, sets_convention_based_database_path_when_autodetecting) {
    char optionsFilename[1000];
    char sectionName[1000];
    FILE file;

    // AUTO-DETECT mode should set cxFileLocation to <projectRoot>/.c-xref/db
    // after applyConventionBasedDatabasePath() is called
    always_expect(getEnv, will_return("HOME"));
    expect(isDirectory, will_return(false));
    expect(directoryName_static, will_return("/home/user/myproject"));
    expect(fileExists, will_return(true));

    expect(openFile, when(fileName, is_equal_to_string("/home/user/myproject/.c-xrefrc")),
           will_return(&file));

    expect_characters("[myproject", false);
    expect(readChar, will_return(']'));

    expect(closeFile);

    searchForProjectOptionsFileAndProjectForFile("/home/user/myproject/source.c",
                                               optionsFilename, sectionName);

    /* Simulate that initStandardCxrefFileName set the default path */
    options.cxFileLocation = "CXrefs";

    /* Now apply the convention-based path */
    applyConventionBasedDatabasePath();

    assert_that(options.detectedProjectRoot, is_equal_to_string("/home/user/myproject"));
    assert_that(options.cxFileLocation, is_equal_to_string("/home/user/myproject/.c-xref/db"));
}

Ensure(Options, uses_directory_basename_as_project_name_for_empty_config) {
    char optionsFilename[1000];
    char sectionName[1000];
    FILE file;

    // AUTO-DETECT mode with empty .c-xrefrc - project name defaults to directory basename
    always_expect(getEnv, will_return("HOME"));
    expect(isDirectory, will_return(false));
    expect(directoryName_static, will_return("/home/user/myproject"));
    expect(fileExists, will_return(true)); /* /home/user/myproject/.c-xrefrc */

    expect(openFile, when(fileName, is_equal_to_string("/home/user/myproject/.c-xrefrc")),
           will_return(&file));

    /* Empty file - just returns EOF */
    expect(readChar, will_return(EOF));

    expect(closeFile);

    /* simpleFileName extracts basename from directory path */
    expect(simpleFileName, will_return("myproject"));

    searchForProjectOptionsFileAndProjectForFile("/home/user/myproject/source.c",
                                               optionsFilename, sectionName);

    /* Project name should be basename of the directory */
    assert_that(sectionName, is_equal_to_string("myproject"));
}

Ensure(Options, collects_option_field_that_allocate_a_string_in_options_space) {
    allocateStringForOption(&options.pushName, "pushName");

    assert_that(containsPointerLocation(options.allUsedStringOptions, (void **)&options.pushName));
    assert_that(options.pushName, is_equal_to_string("pushName"));
    assert_that(memoryIsBetween(&options.memory, options.pushName, 0, options.memory.index));

    /* Add another option and ensure that the string is allocated in option memory... */
    allocateStringForOption(&options.compiler, "compiler");
    assert_that(memoryIsBetween(&options.memory, options.compiler, 0, options.memory.index));
    /* ... and that the option is collected ...*/
    assert_that(containsPointerLocation(options.allUsedStringOptions, (void **)&options.compiler));

    /* ... and finally that the list nodes for allOptionFieldsPointingToAllocatedAreas are also in options memory. */
    assert_that(memoryIsBetween(&options.memory, options.allUsedStringOptions, 0, options.memory.index));
    assert_that(memoryIsBetween(&options.memory, nextPointerLocationList(options.allUsedStringOptions), 0, options.memory.index));
}

Ensure(Options, collects_variable_as_allocating_two_strings_in_options_space) {
    setOptionVariable("ENV", "env");

    assert_that(containsPointerLocation(options.allUsedStringOptions,
                                        (void **)&options.variables[options.variablesCount-1].name));
    assert_that(containsPointerLocation(options.allUsedStringOptions,
                                        (void **)&options.variables[options.variablesCount-1].value));
    assert_that(memoryIsBetween(&options.memory, options.variables[0].name, 0, options.memory.index));
    assert_that(memoryIsBetween(&options.memory, options.variables[0].value, 0, options.memory.index));
    assert_that(getOptionVariable("ENV"), is_equal_to_string("env"));
}


Ensure(Options, collects_option_field_that_allocate_a_string_list_in_options_space) {
    addToStringListOption(&options.includeDirs, "includeDir1");

    assert_that(containsPointerLocation(options.allUsedStringListOptions, (void **)&options.includeDirs));
    assert_that(options.includeDirs->string, is_equal_to_string("includeDir1"));
    assert_that(
        memoryIsBetween(&options.memory, options.includeDirs, 0, options.memory.index));
    assert_that(memoryIsBetween(&options.memory, options.includeDirs->string, 0, options.memory.index));

    addToStringListOption(&options.includeDirs, "includeDir2");
    assert_that(pointerLocationOf(options.allUsedStringListOptions), is_equal_to(&options.includeDirs));
    /* Order of strings is important in options */
    assert_that(options.includeDirs->string, is_equal_to_string("includeDir1"));
    assert_that(options.includeDirs->next->string, is_equal_to_string("includeDir2"));
}

Ensure(Options, can_deep_copy_options_with_one_string_option) {
    allocateStringForOption(&options.compiler, "compiler");

    Options copy = {.memory = {.area = NULL, .size = 0}};
    deepCopyOptionsFromTo(&options, &copy);

    assert_that(options.compiler, is_equal_to_string(copy.compiler)); /* Strings are the same */
    assert_that(options.compiler, is_not_equal_to(copy.compiler)); /* But they are stored in different locations... */
    assert_that(memoryIsBetween(&copy.memory, copy.compiler, 0, copy.memory.index)); /* ... in the copy's memory */
}

Ensure(Options, can_deep_copy_options_with_two_string_options) {
    allocateStringForOption(&options.compiler, "compiler");
    allocateStringForOption(&options.pushName, "pushName");

    Options copy = {.memory = {.area = NULL, .size = 0}};
    deepCopyOptionsFromTo(&options, &copy);

    assert_that(options.pushName, is_equal_to_string(copy.pushName)); /* Strings are the same */
    assert_that(options.pushName, is_not_equal_to(copy.pushName)); /* But they are stored in different locations... */
    assert_that(memoryIsBetween(&copy.memory, copy.pushName, 0, copy.memory.index)); /* ... in the copy's memory */
}

Ensure(Options, can_deep_copy_options_with_one_stringlist_option_with_one_string) {
    addToStringListOption(&options.includeDirs, ".");

    Options copy = {.memory = {.area = NULL, .size = 0}};
    deepCopyOptionsFromTo(&options, &copy);

    /* Strings are the same */
    assert_that(options.includeDirs->string, is_equal_to_string(copy.includeDirs->string));
    /* And they are stored in different locations... */
    assert_that(options.includeDirs->string, is_not_equal_to(copy.includeDirs->string));
    /* ... in the copy's memory */
    assert_that(memoryIsBetween(&copy.memory, copy.includeDirs->string, 0, copy.memory.index));
}

Ensure(Options, can_deep_copy_options_with_one_stringlist_option_with_two_strings) {
    addToStringListOption(&options.includeDirs, ".");
    addToStringListOption(&options.includeDirs, "include");

    Options copy = {.memory = {.area = NULL, .size = 0}};
    deepCopyOptionsFromTo(&options, &copy);

    /* Strings are the same */
    assert_that(options.includeDirs->next->string, is_equal_to_string(copy.includeDirs->next->string));
    /* And they are stored in different locations... */
    assert_that(options.includeDirs->next->string, is_not_equal_to(copy.includeDirs->next->string));
    /* ... in the copy's memory */
    assert_that(memoryIsBetween(&copy.memory, copy.includeDirs->next->string, 0, copy.memory.index));
}

Ensure(Options, can_deep_copy_options_with_two_stringlist_option_with_two_strings) {
    addToStringListOption(&options.includeDirs, ".");
    addToStringListOption(&options.includeDirs, "include");
    addToStringListOption(&options.pruneNames, "prune1");
    addToStringListOption(&options.pruneNames, "prune2");

    Options copy = {.memory = {.area = NULL, .size = 0}};
    deepCopyOptionsFromTo(&options, &copy);

    /* Strings are the same */
    assert_that(options.pruneNames->next->string, is_equal_to_string(copy.pruneNames->next->string));
    /* And they are stored in different locations... */
    assert_that(options.pruneNames->next->string, is_not_equal_to(copy.pruneNames->next->string));
    /* ... in the copy's memory */
    assert_that(memoryIsBetween(&copy.memory, copy.pruneNames->next->string, 0, copy.memory.index));
}

Ensure(Options, can_parse_parameter_name_option) {
    char* arguments[] = {"", "-rfct-parameter-name=int arg"};
    ArgumentsVector args = {2, arguments};

    processOptions(args, false);
    assert_that(options.refactor_parameter_name, is_equal_to_string("int arg"));
}

Ensure(Options, can_parse_parameter_value_option) {
    char* arguments[] = {"", "-rfct-parameter-value=42"};
    ArgumentsVector args = {2, arguments};

    processOptions(args, false);
    assert_that(options.refactor_parameter_value, is_equal_to_string("42"));
}

Ensure(Options, can_parse_target_line_option) {
    char* arguments[] = {"", "-rfct-target-line=99"};
    ArgumentsVector args = {2, arguments};

    processOptions(args, false);
    assert_that(options.refactor_target_line, is_equal_to_string("99"));
}
