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


Ensure(ReferenceRefresh, buildKnowledgeOfCU_should_remove_old_refs_then_parses) {
    FileItem fileItem = {.name = "test.c"};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return(&fileItem));
    expect(editorFileExists, when(path, is_equal_to_string("test.c")), will_return(true));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    /* initializeFileProcessing returns false → no parse attempt */
    expect(initializeFileProcessing, will_return(false));
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expect(editorFileModificationTime, will_return(&contentTime));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_records_the_modification_time_of_what_it_parsed) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return(&fileItem));
    expect(editorFileExists, will_return(true));
    expect(removeReferenceableItemsForFile);
    expect(initializeFileProcessing, will_return(false));
    expect(editorFileModificationTime, when(path, is_equal_to_string("test.c")),
           will_return(&contentTime));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);

    assert_that(fileTimestampsEqual(fileItem.lastParsedMtime, contentTime));
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_marks_a_file_gone_from_disk_as_deleted) {
    FileItem fileItem = {.name = "test.c"};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return(&fileItem));
    expect(editorFileExists, when(path, is_equal_to_string("test.c")), will_return(false));
    expect(markFileAsDeleted, when(fileNumber, is_equal_to(42)));
    never_expect(initializeFileProcessing);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);
}
