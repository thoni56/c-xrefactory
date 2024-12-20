#include <cgreen/cgreen.h>

#include "constants.h"
#include "lexem.h"
#include "lexembuffer.h"
#include "memory.h"
#include "yylex.h"
/* Declare semi-private functions */
void processDefineDirective(bool hasArguments);
void processLineDirective(void);
void processIncludeDirective(Position *includePosition, bool include_next);
void processIncludeNextDirective(Position *includePosition);

#include "filedescriptor.h"
#include "filetable.h"
#include "log.h"
#include "macroargumenttable.h"
#include "symboltable.h"

#include "reference.mock"
#include "c_parser.mock"
#include "caching.mock"
#include "cexp_parser.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "extract.mock"
#include "fileio.mock"
#include "globals.mock"
#include "input.mock"
#include "lexer.mock"
#include "lexem.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "reftab.mock"
#include "semact.mock"
#include "stackmemory.h"
#include "yacc_parser.mock"


Describe(Yylex);
BeforeEach(Yylex) {
    log_set_level(LOG_ERROR);
    initOuterCodeBlock();

    options.mode = ServerMode;
    allocateMacroArgumentTable(MAX_MACRO_ARGS);
    initFileTable(100);
    initNoFileNumber();

    initSymbolTable(100);

    memoryInit(&ppmMemory, "", NULL, SIZE_ppmMemory);

    always_expect(initCharacterBuffer);
}
AfterEach(Yylex) {}

static void setup_lexBuffer_for_reading_identifier(void *data) {
    char *lexemStreamP = currentFile.lexemBuffer.lexemStream;

    /* TODO: yylex does a lot of fishy stuff with the lexems instead
     * of using a LexemBuffer, so here we do as yylex does, although
     * mis-using the LexemBuffer interface */
    putLexemCodeAt(IDENTIFIER, &lexemStreamP);

    strcpy(lexemStreamP, currentFile.characterBuffer.chars);
    /* TODO: WTF This is mostly guesswork, no idea if this is how they are connected... */
    *strchr(&currentFile.lexemBuffer.lexemStream[2], ' ') = '\0';
    currentFile.lexemBuffer.read                          = currentFile.lexemBuffer.lexemStream;
    currentFile.lexemBuffer.write                         = strchr(currentFile.lexemBuffer.lexemStream, '\0');
}

Ensure(Yylex, add_a_cpp_definition_to_the_symbol_table) {
#define DEFINE "__x86_64__"
    FILE *some_file = (FILE *)0x46246546;
    char *definition = (char *)malloc(strlen(DEFINE) + 1);
    strcpy(definition, DEFINE);

    expect(buildLexemFromCharacters, when(buffer, is_equal_to(&currentFile.lexemBuffer)), will_return(true),
           with_side_effect(setup_lexBuffer_for_reading_identifier, NULL));
    expect(setGlobalFileDepNames, when(iname, is_equal_to_string(definition)),
           will_set_contents_of_parameter(pp_name, &definition, sizeof(char *)));
    expect(buildLexemFromCharacters, when(buffer, is_equal_to(&currentFile.lexemBuffer)), will_return(false));

    /* This is the confirmation that there is a symbol p with a
     * field with name equal to DEFINE
     */
    expect(addCxReference, when(symbol_name, is_equal_to_string(DEFINE)));

    /* If the define does not have a body, add the value of "1" */
    initInput(some_file, NULL, "__x86_64__ 1", NULL);
    currentFile.lineNumber = 1;
    processDefineDirective(false);
}

Ensure(Yylex, can_handle_a_line_directive_without_number) {
    expect(buildLexemFromCharacters, when(buffer, is_equal_to(&currentFile.lexemBuffer)), will_return(false));

    initInput(NULL, NULL, "", NULL);
    currentFile.lineNumber = 1;
    processLineDirective();
    /* No asserts, only for execution of END_OF_FILE_EXCEPTION in #line directive */
}

#if CGREEN_VERSION_MINOR < 5
#include "cgreen_capture_parameter.c"
#endif

Ensure(Yylex, can_process_include_directive) {
    Position position     = (Position){1, 2, 3};
    FILE     file;
    int      fileNumber;
    char    lexem_stream[100];

    lexem_stream[0] = (unsigned)STRING_LITERAL%256;
    lexem_stream[1] = STRING_LITERAL/256;
    strcpy(&lexem_stream[2], "\"include.h");

    strcpy(currentFile.lexemBuffer.lexemStream, lexem_stream);
    currentFile.lexemBuffer.read = currentFile.lexemBuffer.lexemStream;
    currentFile.lexemBuffer.write  = currentFile.lexemBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.write = currentInput.begin + strlen(lexem_stream);

    expect(extractPathInto, will_set_contents_of_parameter(dest, "some/path", 10));
    always_expect(normalizeFileName_static, will_return("some/path/include.h"));

    /* Editor does not have the file open... */
    expect(findEditorBufferForFile, when(name, is_equal_to_string("some/path/include.h")),
           will_return(NULL));
    /* ... so open it directly */
    expect(openFile, when(fileName, is_equal_to_string("some/path/include.h")), will_return(&file));

    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2)); /* Don't know why two times... */

    processIncludeDirective(&position, false);
}

Ensure(Yylex, can_process_include_directive_with_include_paths_match_in_second) {
    Position position     = (Position){1, 2, 3};
    int      fileNumber   = 0;
    FILE     file;
    char    lexem_stream[100];

    lexem_stream[0] = (unsigned)STRING_LITERAL%256;
    lexem_stream[1] = STRING_LITERAL/256;
    strcpy(&lexem_stream[2], "\"include.h");

    strcpy(cwd, "cwd");

    strcpy(currentFile.lexemBuffer.lexemStream, lexem_stream);
    currentFile.lexemBuffer.read = currentFile.lexemBuffer.lexemStream;
    currentFile.lexemBuffer.write  = currentFile.lexemBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.write = currentInput.begin + strlen(lexem_stream);

    /* Setup include paths */
    options.includeDirs = newStringList("path1", newStringList("path2", NULL));

    /* Setup so that we are reading file "path1/include.h" */
    currentFile.fileName = "path1/include.h";
    expect(extractPathInto, when(source, is_equal_to_string("path1/include.h")),
           will_set_contents_of_parameter(dest, "path1", sizeof(char *)));

    /* First look in directory of file with the #include since it's not an angle bracketed include */
    expect(normalizeFileName_static, when(name, is_equal_to_string("include.h")),
           when(relative_to, is_equal_to_string("path1")), will_return("path1/include.h"));
    /* Editor should not have any file open... */
    always_expect(findEditorBufferForFile, will_return(NULL));
    /* ... and it should not exist in cwd either... */
    expect(openFile, when(fileName, is_equal_to_string("path1/include.h")), will_return(NULL));

    /* So now we search through include paths (options.includeDirs) which may include wildcards */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path1")), will_return("path1/include.h"));
    expect(expandWildcardsInOnePath, when(filename, is_equal_to_string("path1/include.h")),
           will_set_contents_of_parameter(outpaths, "path1", sizeof(char *)));
    /* Not found in this directory... */
    expect(openFile, when(fileName, is_equal_to_string("path1/include.h")), will_return(NULL));

    expect(normalizeFileName_static, when(name, is_equal_to_string("path2")), will_return("path2/include.h"));
    expect(expandWildcardsInOnePath, when(filename, is_equal_to_string("path2/include.h")),
           will_set_contents_of_parameter(outpaths, "path2", sizeof(char *)));
    /* But in this... */
    expect(openFile, when(fileName, is_equal_to_string("path2/include.h")), will_return(&file));

    /* found: */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path2/include.h")), will_return("path2/include.h"));

    /* Yet another normalization in addFileTableItem()... */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path2/include.h")), will_return("path2/include.h"));

    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2)); /* Don't know why two times... */

    processIncludeDirective(&position, false);
}

Ensure(Yylex, can_process_include_next_directive_and_find_next_with_same_name) {
    Position position     = (Position){1, 2, 3};
    int      fileNumber   = 0;
    FILE     file;
    char     lexem_stream[100];

    lexem_stream[0] = (unsigned)STRING_LITERAL%256;
    lexem_stream[1] = STRING_LITERAL/256;
    strcpy(&lexem_stream[2], "\"include.h");
    strcpy(cwd, "cwd");

    strcpy(currentFile.lexemBuffer.lexemStream, lexem_stream);
    currentFile.lexemBuffer.read = currentFile.lexemBuffer.lexemStream;
    currentFile.lexemBuffer.write  = currentFile.lexemBuffer.lexemStream + strlen(lexem_stream);

    initInput(NULL, NULL, "", NULL);
    currentInput.write = currentInput.begin + strlen(lexem_stream);

    /* Setup include paths */
    options.includeDirs = newStringList("path1", newStringList("path2", newStringList("path3", NULL)));

    /* Setup so that we are reading file "path2/include.h" */
    currentFile.fileName = "path2/include.h";
    expect(extractPathInto, when(source, is_equal_to_string("path2/include.h")),
           will_set_contents_of_parameter(dest, "path2/", sizeof(char *)));

    /* First find which include path the current file was found at... */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path1")),
           when(relative_to, is_equal_to_string("cwd")), will_return("path1"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("path2")),
           when(relative_to, is_equal_to_string("cwd")), will_return("path2"));

    /* So now we search through include paths (options.includeDirs) starting at the one after path2... */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path3")),
           when(relative_to, is_equal_to_string("cwd")), will_return("path3/include.h"));
    /* And it might contain wildcards, so ... */
    expect(expandWildcardsInOnePath, when(filename, is_equal_to_string("path3/include.h")),
           will_set_contents_of_parameter(outpaths, "path3", sizeof(char *)));
    /* Editor should not have any file open... */
    always_expect(findEditorBufferForFile, will_return(NULL));
    /* ... but opening it works */
    expect(openFile, when(fileName, is_equal_to_string("path3/include.h")), will_return(&file));

    /* found: */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path3/include.h")),
           when(relative_to, is_equal_to_string("cwd")), will_return("path3/include.h"));

    /* Yet another normalization in addFileTableItem()... */
    expect(normalizeFileName_static, when(name, is_equal_to_string("path3/include.h")), will_return("path3/include.h"));

    /* Always  */
    always_expect(checkFileModifiedTime);
    always_expect(cacheInclude, will_capture_parameter(fileNum, fileNumber));

    /* Finally ensure that the include file is added as a reference */
    expect(addCxReference, when(symbol_name, is_equal_to_string(LINK_NAME_INCLUDE_REFS)),
           times(2)); /* Don't know why two times... */

    processIncludeDirective(&position, true);
}
