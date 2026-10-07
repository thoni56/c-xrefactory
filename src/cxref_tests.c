#include "cxref.h"

/* Unittests for Cxref */

#include <cgreen/cgreen.h>

#include "log.h"
#include "protocol.h"
#include "browsingmenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "completion.mock"
#include "dump.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "editorbuffertable.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "lexer.mock"
#include "match.mock"
#include "misc.mock"
#include "navigation.mock"
#include "referencerefresh.mock"
#include "options.mock"
#include "parsers.mock"
#include "parsing.mock"
#include "ppc.mock"
#include "progress.mock"
#include "refactorings.mock"
#include "refactory.mock"
#include "reference.mock"
#include "referenceableitemtable.mock"
#include "search.mock"
#include "session.mock"
#include "server.mock"
#include "startup.mock"
#include "symbol.mock"
#include "yylex.mock"


Describe(CxRef);
BeforeEach(CxRef) {
    log_set_level(LOG_ERROR);
}
AfterEach(CxRef) {}


Ensure(CxRef, can_parse_line_and_col_from_command_line_option) {
    int line, column;
    options.cursorLineColumn = "54:33";
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &column);
    assert_that(line, is_equal_to(54));
    assert_that(column, is_equal_to(33));
}

Ensure(CxRef, will_return_no_active_project_if_no_optionfile_found) {
    FileItem fileItem = {.name = "file.c"};

    options.cxrefProtocol = true;
    outputFile = stdout;
    options.serverOperation = OP_GET_PROJECT;

    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(0)),
           will_return(&fileItem));
    expect(searchForProjectConfigFileAndProjectForFile,
           when(sourceFilename, is_equal_to_string("file.c")),
           will_set_contents_of_parameter(foundConfigFilename, "", 1),
           will_set_contents_of_parameter(foundProjectName, "", 1));
    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_NO_PROJECT)),
           when(message, contains_string("file.c")));

    answerEditorAction();
}

/* Tripwire: -p is suspected redundant, the client echoing back the lock. These
 * pin the cases where ignoring -p would change the answer. */

Ensure(CxRef, warns_when_request_names_a_project_but_none_is_locked) {
    options.cxrefProtocol = true;
    outputFile = stdout;
    options.serverOperation = OP_GET_PROJECT;
    options.project = "/some/project";
    lockedProject = NULL;

    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_BOTTOM_WARNING)),
           when(message, contains_string("/some/project")));
    expect(ppcGenRecord, when(kind, is_equal_to(PPC_SET_INFO)));

    answerEditorAction();

    options.project = NULL;
}

Ensure(CxRef, warns_when_request_names_another_project_than_the_locked_one) {
    options.cxrefProtocol = true;
    outputFile = stdout;
    options.serverOperation = OP_GET_PROJECT;
    options.project = "/other/project";
    lockedProject = "/some/project";

    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_BOTTOM_WARNING)),
           when(message, contains_string("/other/project")));
    expect(ppcGenRecord, when(kind, is_equal_to(PPC_SET_INFO)));

    answerEditorAction();

    options.project = NULL;
    lockedProject = NULL;
}

Ensure(CxRef, does_not_warn_when_request_names_the_locked_project) {
    options.cxrefProtocol = true;
    outputFile = stdout;
    options.serverOperation = OP_GET_PROJECT;
    options.project = "/some/project";
    lockedProject = "/some/project";

    expect(ppcGenRecord, when(kind, is_equal_to(PPC_SET_INFO)));

    answerEditorAction();

    options.project = NULL;
    lockedProject = NULL;
}
