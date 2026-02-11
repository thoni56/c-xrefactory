#include "cxref.h"

/* Unittests for Cxref */

#include <cgreen/cgreen.h>

#include "log.h"
#include "protocol.h"
#include "browsermenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "completion.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "lexer.mock"
#include "match.mock"
#include "misc.mock"
#include "navigation.mock"
#include "options.mock"
#include "parsers.mock"
#include "parsing.mock"
#include "ppc.mock"
#include "refactorings.mock"
#include "refactory.mock"
#include "reference.mock"
#include "referenceableitemtable.mock"
#include "search.mock"
#include "session.mock"
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
    options.olcxlccursor = "54:33";
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &column);
    assert_that(line, is_equal_to(54));
    assert_that(column, is_equal_to(33));
}

Ensure(CxRef, will_return_no_active_project_if_no_optionfile_found) {
    FileItem fileItem = {.name = "file.c"};

    options.xref2 = true;
    outputFile = stdout;
    options.serverOperation = OP_ACTIVE_PROJECT;

    expect(applyConventionBasedDatabasePath);
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(0)),
           will_return(&fileItem));
    expect(searchForProjectOptionsFileAndProjectForFile, when(sourceFilename, is_equal_to_string("file.c")),
           will_set_contents_of_parameter(foundOptionsFilename, "", 1),
           will_set_contents_of_parameter(foundProjectName, "", 1));
    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_NO_PROJECT)),
           when(message, contains_string("file.c")));

    answerEditorAction();
}
