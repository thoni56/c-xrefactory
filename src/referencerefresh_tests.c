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

Ensure(ReferenceRefresh, ensureFreshReferences_should_do_nothing_for_item_with_no_references) {
    ReferenceableItem item = makeReferenceableItem("foo", TypeDefault, StorageDefault,
                                                   GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);
    item.references = NULL;

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    ensureFreshReferences(&item, baseArgs);
}

Ensure(ReferenceRefresh, ensureFreshReferences_should_not_reparse_when_file_is_fresh) {
    Reference ref = {.position = {.file = 1, .line = 5, .col = 0}, .usage = UsageDefined, .next = NULL};
    ReferenceableItem item = makeReferenceableItem("foo", TypeDefault, StorageDefault,
                                                   GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);
    item.references = &ref;

    FileTimestamp mtime100 = {100, 0};
    FileItem fileItem = {.name = "foo.c", .lastParsedMtime = {100, 0}};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("foo.c")),
           will_return(&mtime100));
    never_expect(parseToCreateReferences);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    ensureFreshReferences(&item, baseArgs);
}

Ensure(ReferenceRefresh, ensureFreshReferences_should_reparse_when_file_is_stale) {
    Reference ref = {.position = {.file = 1, .line = 5, .col = 0}, .usage = UsageDefined, .next = NULL};
    ReferenceableItem item = makeReferenceableItem("foo", TypeDefault, StorageDefault,
                                                   GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);
    item.references = &ref;

    FileTimestamp mtime200 = {200, 0};
    FileItem fileItem = {.name = "foo.c", .lastParsedMtime = {100, 0}};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("foo.c")),
           will_return(&mtime200));
    /* Stale: mtime 200 != lastParsedMtime 100 → should reparse */
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(1)));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences);
    expect(closeCharacterBuffer);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    ensureFreshReferences(&item, baseArgs);
}

Ensure(ReferenceRefresh, ensureFreshReferences_should_reparse_stale_file_only_once_for_multiple_references) {
    Reference ref2 = {.position = {.file = 1, .line = 20, .col = 0}, .usage = UsageUsed, .next = NULL};
    Reference ref1 = {.position = {.file = 1, .line = 5, .col = 0}, .usage = UsageDefined, .next = &ref2};
    ReferenceableItem item = makeReferenceableItem("foo", TypeDefault, StorageDefault,
                                                   GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);
    item.references = &ref1;

    FileTimestamp mtime200 = {200, 0};
    FileItem fileItem = {.name = "foo.c", .lastParsedMtime = {100, 0}};
    /* Should only be called once despite two references in the same file */
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("foo.c")),
           will_return(&mtime200));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(1)));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences);
    expect(closeCharacterBuffer);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    ensureFreshReferences(&item, baseArgs);
}

Ensure(ReferenceRefresh, ensureFreshReferences_should_reparse_when_file_was_never_parsed) {
    Reference ref = {.position = {.file = 1, .line = 5, .col = 0}, .usage = UsageDefined, .next = NULL};
    ReferenceableItem item = makeReferenceableItem("foo", TypeDefault, StorageDefault,
                                                   GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);
    item.references = &ref;

    FileTimestamp mtime100 = {100, 0};
    FileItem fileItem = {.name = "foo.c", .lastParsedMtime = {0, 0}};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("foo.c")),
           will_return(&mtime100));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(1)));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&fileItem));
    expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences);
    expect(closeCharacterBuffer);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    ensureFreshReferences(&item, baseArgs);
}


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
