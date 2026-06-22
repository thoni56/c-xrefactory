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

Ensure(Server, has_a_none_operation) {
    assert_that(OP_NONE, is_equal_to(0));
    assert_that(operationNamesTable[OP_NONE], is_equal_to_string("OP_NONE"));
}

/* Suspended: describes the bug we still need to fix.
 *
 * prepareInputFileForRequest() currently picks the first scheduled file by
 * fileNumber order. fileNumber is hash(absolute path), so when an unrelated
 * file ends up scheduled alongside the request file (observed via .c-xrefrc
 * re-expansion), the wrong file can win the tie-break depending on which
 * path hashes lower. On macOS this caused test_browsing_push_in_unexpanded_macro
 * to fail; on Linux the same code passed by accident of path-prefix hashing.
 *
 * The desired behaviour is: prepare the request file regardless of what else
 * happens to be scheduled. A naive attempt using options.inputFiles broke ~50
 * other tests, so the right resolution path is still open. */
xEnsure(Server, prepares_the_request_file_when_others_are_also_scheduled) {
    int otherFile = 99;
    int requestFile = 100;
    FileItem fileItem = { .isScheduled = true };

    expect(getNextScheduledFile,
           will_set_contents_of_parameter(beginNumber, &otherFile, sizeof(int)),
           will_return("other_file.c"));
    expect(getFileItemWithFileNumber,
           when(fileNumber, is_equal_to(otherFile)),
           will_return(&fileItem));
    expect(getNextExistingFileNumber, will_return(requestFile));
    expect(getFileItemWithFileNumber,
           when(fileNumber, is_equal_to(requestFile)),
           will_return(&fileItem));
    expect(getNextExistingFileNumber, will_return(-1));

    assert_that(prepareInputFileForRequest(), is_true);
    assert_that(requestFileNumber, is_equal_to(requestFile));
}
