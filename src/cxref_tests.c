#include <cgreen/cgreen.h>

#include "cxref.h"

#include "log.h"
#include "memory.h"

/* Dependencies: */
#include "session.h"

#include "caching.mock"
#include "characterreader.mock"
#include "classfilereader.mock"
#include "classhierarchy.mock"
#include "commons.mock"
#include "complete.mock"
#include "completion.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
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
    olcxInit();
}
AfterEach(CxRef) {}

Ensure(CxRef, get_class_num_from_class_linkname_will_return_default_value_if_not_member) {
    int defaultValue = 14;

    expect(existsInFileTable, when(fileName, is_equal_to_string(";name.class")), will_return(false));
    expect(convertLinkNameToClassFileName, when(linkName, is_equal_to_string("name")),
           will_set_contents_of_parameter(classFileName, ";name.class", 12));

    assert_that(getClassNumFromClassLinkName("name", defaultValue), is_equal_to(defaultValue));
}

Ensure(CxRef, get_class_num_from_class_linkname_will_return_filenumber_if_member) {
    int defaultValue = 14;
    int position     = 42;

    expect(existsInFileTable, when(fileName, is_equal_to_string(";name.class")), will_return(true));
    expect(lookupFileTable, will_return(position));
    expect(convertLinkNameToClassFileName, when(linkName, is_equal_to_string("name")),
           will_set_contents_of_parameter(classFileName, ";name.class", 12));

    assert_that(getClassNumFromClassLinkName("name", defaultValue), is_equal_to(position));
}

Ensure(CxRef, can_parse_line_and_col_from_command_line_option) {
    int line, column;
    options.olcxlccursor = "54:33";
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &column);
    assert_that(line, is_equal_to(54));
    assert_that(column, is_equal_to(33));
}

Ensure(CxRef, can_get_active_project) {
    FileItem fileItem = {.name = "file.c"};

    options.xref2 = true;
    communicationChannel = stdout;
    options.serverOperation = OLO_ACTIVE_PROJECT;

    expect(getFileItem, when(fileIndex, is_equal_to(0)),
           will_return(&fileItem));
    expect(searchStandardOptionsFileAndProjectForFile, when(fileName, is_equal_to_string("file.c")));
    expect(ppcGenRecord,
           when(kind, is_equal_to(PPC_NO_PROJECT)),
           when(message, contains_string("file.c")));

    answerEditAction();
}
