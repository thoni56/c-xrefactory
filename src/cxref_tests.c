#include <cgreen/cgreen.h>

#include "cxref.h"

#include "log.h"

/* Dependencies: */
#include "caching.mock"
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
#include "main.mock"
#include "menu.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "refactory.mock"
#include "refactorings.mock"
#include "reference.mock"
#include "reftab.mock"
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
    communicationChannel = stdout;
    options.serverOperation = OLO_ACTIVE_PROJECT;

    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(0)),
           will_return(&fileItem));
    expect(searchStandardOptionsFileAndProjectForFile, when(sourceFilename, is_equal_to_string("file.c")),
           will_set_contents_of_parameter(foundOptionsFilename, "", 1),
           will_set_contents_of_parameter(foundProjectName, "", 1));
    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_NO_PROJECT)),
           when(message, contains_string("file.c")));

    answerEditAction();
}
