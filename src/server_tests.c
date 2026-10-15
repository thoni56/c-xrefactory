#include "server.h"

/* Unittests for Server */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include <time.h>

#include "log.h"

#include "browsingmenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "completion.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "editorbuffertable.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "freshness.mock"
#include "includegraph.mock"
#include "globals.mock"
#include "init.mock"
#include "lexer.mock"
#include "macroargumenttable.mock"
#include "misc.mock"
#include "navigation.mock"
#include "referencerefresh.mock"
#include "options.mock"
#include "parsers.mock"
#include "parsing.mock"
#include "ppc.mock"
#include "progress.mock"
#include "projectstructure.mock"
#include "rebuild.mock"
#include "refactorings.mock"
#include "refactory.mock"
#include "reference.mock"
#include "referenceableitemtable.mock"
#include "session.mock"
#include "startup.mock"
#include "symboltable.mock"
#include "type.mock"
#include "yacc_parser.mock"
#include "yylex.mock"


Describe(Server);
BeforeEach(Server) {
    log_set_level(LOG_ERROR);
}
AfterEach(Server) {}

/* Protected */
extern bool prepareInputFileForRequest(void);
extern void setRequestFileArgument(int fileNumber);
extern void singlePass(ArgumentsVector args, ArgumentsVector nargs);
extern bool needsConfirmationToAnalyse(int outOfDateCUs);

Ensure(Server, has_a_none_operation) {
    assert_that(OP_NONE, is_equal_to(0));
    assert_that(operationNamesTable[OP_NONE], is_equal_to_string("OP_NONE"));
}

Ensure(Server, does_not_ask_before_analysing_files_within_the_cap) {
    assert_that(needsConfirmationToAnalyse(MAX_CUS_TO_REPARSE), is_false);
}

Ensure(Server, asks_before_analysing_more_files_than_the_cap) {
    assert_that(needsConfirmationToAnalyse(MAX_CUS_TO_REPARSE + 1), is_true);
}

/* A request has at most one input file, the one it named */
Ensure(Server, prepares_the_file_the_request_named) {
    int requestFile = 100;

    setRequestFileArgument(requestFile);

    assert_that(prepareInputFileForRequest(), is_true);
    assert_that(requestFileNumber, is_equal_to(requestFile));
}

Ensure(Server, prepares_no_file_when_the_request_named_none) {
    setRequestFileArgument(NO_FILE_NUMBER);

    assert_that(prepareInputFileForRequest(), is_false);
    assert_that(requestFileNumber, is_equal_to(NO_FILE_NUMBER));
}

/* sideEffect, if not NULL, runs when the parser is called */
static void expectACursorParseOf(FileItem *fileItem, FileTimestamp *contentTime,
                                 void (*sideEffect)(void *)) {
    currentFile.characterBuffer.fileNumber = 42;
    currentFile.characterBuffer.file = stdin;
    inputFileName = fileItem->name;
    options.serverOperation = OP_BROWSE_PUSH;
    options.cursorOffset = 1;

    expect(initializeFileProcessing, will_return(true));
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(fileItem));
    always_expect(editorFileModificationTime, will_return(contentTime));
    expect(setupParsingConfig);
    if (sideEffect != NULL)
        expect(callParser, with_side_effect(sideEffect, NULL));
    else
        expect(callParser);
}

Ensure(Server, cursor_parse_of_a_compilation_unit_records_when_it_built_the_knowledge) {
    FileItem      fileItem    = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectACursorParseOf(&fileItem, &contentTime, NULL);

    ArgumentsVector args = {.argc = 0, .argv = NULL};
    FileTimestamp before = fileTimestampNow();
    singlePass(args, args);
    FileTimestamp after = fileTimestampNow();

    assert_false(fileTimestampIsLessThan(fileItem.knowledgeTime, before));
    assert_false(fileTimestampIsLessThan(after, fileItem.knowledgeTime));
}

static FileTimestamp parseStart;

/* Sleeps after recording, so a time taken after the parse started is later. */
static void recordParseStart(void *unused) {
    parseStart = fileTimestampNow();
    nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
}

Ensure(Server, cursor_parse_takes_the_knowledge_time_before_parsing) {
    FileItem      fileItem    = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectACursorParseOf(&fileItem, &contentTime, recordParseStart);

    ArgumentsVector args = {.argc = 0, .argv = NULL};
    singlePass(args, args);

    assert_false(fileTimestampIsLessThan(parseStart, fileItem.knowledgeTime));
}

Ensure(Server, cursor_parse_for_an_operation_that_does_not_parse_leaves_the_file_unparsed) {
    FileItem      fileItem    = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    currentFile.characterBuffer.fileNumber = 42;
    currentFile.characterBuffer.file = stdin;
    inputFileName = fileItem.name;
    options.serverOperation = OP_SEARCH;
    options.cursorOffset = 1;

    expect(initializeFileProcessing, will_return(true));
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(&fileItem));
    always_expect(editorFileModificationTime, will_return(&contentTime));
    never_expect(removeReferenceableItemsForFile);
    expect(setupParsingConfig);
    never_expect(callParser);

    ArgumentsVector args = {.argc = 0, .argv = NULL};
    singlePass(args, args);

    assert_true(fileTimestampIsZero(fileItem.lastParsedMtime));
    assert_true(fileTimestampIsZero(fileItem.knowledgeTime));
}

/* Search answers from the table, so a stripped request file would lose its references
 * for good */
Ensure(Server, search_does_not_strip_its_request_file) {
    FileItem fileItem = {.name = "a.c"};
    setRequestFileArgument(42);
    options.serverOperation = OP_SEARCH;

    cgreen_mocks_are(loose_mocks);
    always_expect(getFileItemWithFileNumber, will_return(&fileItem));
    expect(initializeProjectContext, will_return(true));
    never_expect(collectRequestCU);

    ArgumentsVector args = {.argc = 0, .argv = NULL};
    callServer(args, args);
}

Ensure(Server, push_collects_its_request_file_for_stripping) {
    FileItem fileItem = {.name = "a.c"};
    setRequestFileArgument(42);
    options.serverOperation = OP_BROWSE_PUSH;

    cgreen_mocks_are(loose_mocks);
    always_expect(getFileItemWithFileNumber, will_return(&fileItem));
    expect(initializeProjectContext, will_return(true));
    expect(collectRequestCU, when(requestFileNumber, is_equal_to(42)));

    ArgumentsVector args = {.argc = 0, .argv = NULL};
    callServer(args, args);
}
