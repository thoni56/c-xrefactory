#include "referencerefresh.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "referenceableitem.h"

#include "commons.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
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
