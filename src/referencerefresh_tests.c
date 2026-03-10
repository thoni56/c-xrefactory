#include "referencerefresh.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"

#include "commons.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "options.mock"
#include "parsing.mock"
#include "referenceableitemtable.mock"
#include "startup.mock"
#include "characterreader.mock"


Describe(ReferenceRefresh);
BeforeEach(ReferenceRefresh) {
    log_set_level(LOG_ERROR);
}
AfterEach(ReferenceRefresh) {}

Ensure(ReferenceRefresh, reparseStaleFile_should_remove_old_refs_then_parses) {
    FileItem fileItem = {.name = "test.c"};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return(&fileItem));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    /* initializeFileProcessing returns false → no parse attempt */
    expect(initializeFileProcessing, will_return(false));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    reparseStaleFile(42, baseArgs);
}

Ensure(ReferenceRefresh, parseFileWithFullInit_should_save_and_restore_request_options) {
    options.cursorOffset = 100;
    options.noErrors = false;
    options.serverOperation = OP_BROWSE_PUSH;

    /* initializeFileProcessing returns false → no parse, but options should still be restored */
    expect(initializeFileProcessing, will_return(false));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    parseFileWithFullInit("test.c", baseArgs);

    assert_that(options.cursorOffset, is_equal_to(100));
    assert_that(options.noErrors, is_equal_to(false));
    assert_that(options.serverOperation, is_equal_to(OP_BROWSE_PUSH));
}

Ensure(ReferenceRefresh, parseFileWithFullInit_should_call_parse_when_init_succeeds) {
    options.cursorOffset = 50;
    options.noErrors = false;
    options.serverOperation = OP_BROWSE_PUSH;

    expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences);
    expect(closeCharacterBuffer);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    parseFileWithFullInit("test.c", baseArgs);

    /* Options restored after parse */
    assert_that(options.cursorOffset, is_equal_to(50));
    assert_that(options.serverOperation, is_equal_to(OP_BROWSE_PUSH));
}
