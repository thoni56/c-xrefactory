#include <cgreen/cgreen.h>

#include "yylex.h"
/* Declare semi-private functions */
void processDefineDirective(bool hasArguments);
void processLineDirective(void);
void processIncludeDirective(Position *includePosition, bool include_next);
void processIncludeNextDirective(Position *includePosition);

#include "filedescriptor.h"
#include "filetable.h"
#include "symboltable.h"
#include "macroargumenttable.h"
#include "symbol.h"
#include "log.h"

#include "globals.mock"
#include "options.mock"
#include "caching.mock"
#include "parsers.mock"
#include "cexp_parser.mock"
#include "jslsemact.mock"
#include "jsemact.mock"
#include "reftab.mock"
#include "editor.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxfile.mock"
#include "characterreader.mock"
#include "lexer.mock"
#include "semact.mock"
#include "java_parser.mock"
#include "cxref.mock"
#include "extract.mock"
#include "c_parser.mock"
#include "yacc_parser.mock"
#include "fileio.mock"


Describe(Yylex);
BeforeEach(Yylex) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();

    options.taskRegime = RegimeEditServer;
    allocateMacroArgumentTable();
    initFileTable(&fileTable);
    symbolTable = StackMemoryAlloc(SymbolTable);
    symbolTableInit(symbolTable, 10);
}
AfterEach(Yylex) {}

static void setup_lexBuffer_for_reading_identifier(void *data) {
    char *lexemStreamP = currentFile.lexBuffer.lexemStream;
    putLexToken(IDENTIFIER, &lexemStreamP);
    strcpy(lexemStreamP, currentFile.lexBuffer.buffer.chars);
    /* TODO: WTF This is mostly guesswork, no idea if this is how they are connected... */
    *strchr(&currentFile.lexBuffer.lexemStream[2], ' ') = '\0';
    currentFile.lexBuffer.next = currentFile.lexBuffer.lexemStream;
    currentFile.lexBuffer.end = strchr(currentFile.lexBuffer.lexemStream, '\0');
    currentFile.lexBuffer.index = 2;
}

Ensure(Yylex, add_a_cpp_definition_to_the_symbol_table) {
#   define DEFINE "__x86_64__"
    char *definition = (char *)malloc(strlen(DEFINE)+1);
    strcpy(definition, DEFINE);

    expect(getLexemFromLexer, when(buffer, is_equal_to(&currentFile.lexBuffer)),
           will_return(true), with_side_effect(setup_lexBuffer_for_reading_identifier, NULL));
    expect(setGlobalFileDepNames, when(iname, is_equal_to_string(definition)),
           will_set_contents_of_parameter(pp_name, &definition, sizeof(char *)));
    expect(getLexemFromLexer, when(buffer, is_equal_to(&currentFile.lexBuffer)),
           will_return(false));

    /* This is the confirmation that there is a symbol p with a
     * field with name equal to DEFINE
     */
    expect(addCxReference, when(symbol_name, is_equal_to_string(DEFINE)));

    /* If the define does not have a body, add the value of "1" */
    initInput(NULL, NULL, "__x86_64__ 1", NULL);
    currentFile.lineNumber = 1;
    processDefineDirective(false);
}

Ensure(Yylex, can_handle_a_line_directive_without_number) {
    expect(getLexemFromLexer, when(buffer, is_equal_to(&currentFile.lexBuffer)),
           will_return(false));

    initInput(NULL, NULL, "", NULL);
    currentFile.lineNumber = 1;
    processLineDirective();
    /* No asserts, only for execution of END_OF_FILE_EXCEPTION in #line directive */
}

/* ----------------------------------------------------*/
/* Custom Cgreen constraint to capture parameter value */

/* We need a copy of ... */

static CgreenValue make_cgreen_pointer_value(void *pointer) {
    CgreenValue value = {CGREEN_POINTER, {0}, sizeof(intptr_t)};
    value.value.pointer_value = pointer;
    return value;
}

/* Start of custom Cgreen constraint... */

static void capture_parameter(Constraint *constraint, const char *function, CgreenValue actual,
                              const char *test_file, int test_line, TestReporter *reporter) {
    (void)function;
    (void)test_file;
    (void)test_line;
    (void)reporter;

    memmove(constraint->expected_value.value.pointer_value, &actual.value, constraint->size_of_expected_value);
}

static bool compare_true() { return true; }

static Constraint *create_capture_parameter_constraint(const char *parameter_name, void *capture_to, size_t size_to_capture) {
    Constraint* constraint = create_constraint();

    constraint->type = CGREEN_VALUE_COMPARER_CONSTRAINT;
    constraint->compare = &compare_true;
    constraint->execute = &capture_parameter;
    constraint->name = "capture parameter";
    constraint->expected_value = make_cgreen_pointer_value(capture_to);
    constraint->size_of_expected_value = size_to_capture;
    constraint->parameter_name = parameter_name;

    return constraint;
}

#define will_capture_parameter(parameter_name, local_variable) create_capture_parameter_constraint(#parameter_name, &local_variable, sizeof(local_variable))

/* End of custom Cgreen constraint */


Ensure(Yylex, can_process_include_directive) {
    Position position = (Position){1,2,3};
    char *lexem_stream = "\303\001\"include.h";
    FILE file;
    int fileNumber;

    strcpy(currentFile.lexBuffer.lexemStream, lexem_stream);
    currentFile.lexBuffer.next = currentFile.lexBuffer.lexemStream;
    currentFile.lexBuffer.end = currentFile.lexBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.endOfBuffer = currentInput.beginningOfBuffer + strlen(lexem_stream);

    expect(extractPathInto, will_set_contents_of_parameter(dest, "some/path", 10));
    always_expect(normalizeFileName,
                  will_return("some/path/include.h"));

    /* Editor does not have the file open... */
    expect(editorFindFile,
           when(name, is_equal_to_string("some/path/include.h")),
           will_return(NULL));
    /* ... so open it directly */
    expect(openFile, when(fileName, is_equal_to_string("some/path/include.h")), will_return(&file));

    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2));           /* Don't know why two times... */

    processIncludeDirective(&position, false);

    assert_that(fileTable.tab[fileNumber]->name, is_equal_to_string("some/path/include.h"));
}

Ensure(Yylex, can_process_include_directive_with_include_paths_match_in_second) {
    Position position = (Position){1,2,3};
    char *lexem_stream = "\303\001\"include.h";
    int fileNumber = 0;
    FILE file;

    strcpy(cwd, "cwd");

    strcpy(currentFile.lexBuffer.lexemStream, lexem_stream);
    currentFile.lexBuffer.next = currentFile.lexBuffer.lexemStream;
    currentFile.lexBuffer.end = currentFile.lexBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.endOfBuffer = currentInput.beginningOfBuffer + strlen(lexem_stream);

    /* Setup include paths */
    options.includeDirs = newStringList("path1", newStringList("path2", NULL));

    /* Setup so that we are reading file "path1/include.h" */
    currentFile.fileName = "path1/include.h";
    expect(extractPathInto,
           when(source, is_equal_to_string("path1/include.h")),
           will_set_contents_of_parameter(dest, "path1", sizeof(char*)));

    /* First look in directory of file with the #include since it's not an angle bracketed include */
    expect(normalizeFileName,
           when(name, is_equal_to_string("include.h")),
           when(relative_to, is_equal_to_string("path1")),
           will_return("path1/include.h"));
    /* Editor should not have any file open... */
    always_expect(editorFindFile, will_return(NULL));
    /* ... and it should not exist in cwd either... */
    expect(openFile, when(fileName, is_equal_to_string("path1/include.h")), will_return(NULL));

    /* So now we search through include paths (options.includeDirs) which may include wildcards */
    expect(normalizeFileName, when(name, is_equal_to_string("path1")), will_return("path1/include.h"));
    expect(expandWildcardsInOnePath,
           when(filename, is_equal_to_string("path1/include.h")),
           will_set_contents_of_parameter(outpaths, "path1", sizeof(char*)));
    /* Not found in this directory... */
    expect(openFile, when(fileName, is_equal_to_string("path1/include.h")), will_return(NULL));

    expect(normalizeFileName, when(name, is_equal_to_string("path2")), will_return("path2/include.h"));
    expect(expandWildcardsInOnePath,
           when(filename, is_equal_to_string("path2/include.h")),
           will_set_contents_of_parameter(outpaths, "path2", sizeof(char*)));
    /* But in this... */
    expect(openFile, when(fileName, is_equal_to_string("path2/include.h")), will_return(&file));

    /* found: */
    expect(normalizeFileName, when(name, is_equal_to_string("path2/include.h")), will_return("path2/include.h"));

    /* Yet another normalization in addFileTabItem()... */
    expect(normalizeFileName, when(name, is_equal_to_string("path2/include.h")), will_return("path2/include.h"));


    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2));           /* Don't know why two times... */

    processIncludeDirective(&position, false);

    assert_that(fileTable.tab[fileNumber]->name, is_equal_to_string("path2/include.h"));
}


Ensure(Yylex, can_process_include_next_directive_and_find_next_with_same_name) {
    Position position = (Position){1,2,3};
    char *lexem_stream = "\303\001\"include.h";
    int fileNumber = 0;
    FILE file;

    strcpy(cwd, "cwd");

    strcpy(currentFile.lexBuffer.lexemStream, lexem_stream);
    currentFile.lexBuffer.next = currentFile.lexBuffer.lexemStream;
    currentFile.lexBuffer.end = currentFile.lexBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.endOfBuffer = currentInput.beginningOfBuffer + strlen(lexem_stream);

    /* Setup include paths */
    options.includeDirs = newStringList("path1", newStringList("path2", newStringList("path3", NULL)));

    /* Setup so that we are reading file "path2/include.h" */
    currentFile.fileName = "path2/include.h";
    expect(extractPathInto,
           when(source, is_equal_to_string("path2/include.h")),
           will_set_contents_of_parameter(dest, "path2/", sizeof(char*)));

    /* First find which include path the current file was found at... */
    expect(normalizeFileName,
           when(name, is_equal_to_string("path1")),
           when(relative_to, is_equal_to_string("cwd")),
           will_return("path1"));
    expect(normalizeFileName,
           when(name, is_equal_to_string("path2")),
           when(relative_to, is_equal_to_string("cwd")),
           will_return("path2"));

    /* So now we search through include paths (options.includeDirs) starting at the one after path2... */
    expect(normalizeFileName,
           when(name, is_equal_to_string("path3")),
           when(relative_to, is_equal_to_string("cwd")),
           will_return("path3/include.h"));
    /* And it might contain wildcards, so ... */
    expect(expandWildcardsInOnePath,
           when(filename, is_equal_to_string("path3/include.h")),
           will_set_contents_of_parameter(outpaths, "path3", sizeof(char*)));
    /* Editor should not have any file open... */
    always_expect(editorFindFile, will_return(NULL));
    /* ... but opening it works */
    expect(openFile, when(fileName, is_equal_to_string("path3/include.h")), will_return(&file));

    /* found: */
    expect(normalizeFileName,
           when(name, is_equal_to_string("path3/include.h")),
           when(relative_to, is_equal_to_string("cwd")),
           will_return("path3/include.h"));

    /* Yet another normalization in addFileTabItem()... */
    expect(normalizeFileName, when(name, is_equal_to_string("path3/include.h")), will_return("path3/include.h"));


    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2));           /* Don't know why two times... */

    processIncludeDirective(&position, true);

    assert_that(fileTable.tab[fileNumber]->name, is_equal_to_string("path3/include.h"));
}
