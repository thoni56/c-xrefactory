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

Ensure(Server, has_a_none_operation) {
    assert_that(OP_NONE, is_equal_to(0));
    assert_that(operationNamesTable[OP_NONE], is_equal_to_string("OP_NONE"));
}

/* The request names a file; other files can be scheduled alongside it when a
 * legacy project config expands its source directories. The request's file is
 * the one to prepare, and it must end up the only one scheduled. */
Ensure(Server, prepares_the_file_the_request_named_when_others_are_also_scheduled) {
    int      otherFile = 99;
    int      requestFile = 100;
    FileItem otherItem = {.isScheduled = true};
    FileItem requestItem = {.isScheduled = true};

    setRequestFileArgument(requestFile);

    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(requestFile)), will_return(&requestItem));
    /* everything else scheduled is unscheduled */
    expect(getNextExistingFileNumber, will_return(otherFile));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(otherFile)), will_return(&otherItem));
    expect(getNextExistingFileNumber, will_return(requestFile));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(requestFile)), will_return(&requestItem));
    expect(getNextExistingFileNumber, will_return(-1));

    assert_that(prepareInputFileForRequest(), is_true);
    assert_that(requestFileNumber, is_equal_to(requestFile));
    assert_that(otherItem.isScheduled, is_false);
    assert_that(requestItem.isScheduled, is_true);
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
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
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
