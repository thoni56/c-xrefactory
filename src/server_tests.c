#include "server.h"

/* Unittests for Server */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

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
