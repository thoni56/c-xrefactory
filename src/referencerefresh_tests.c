#include "referencerefresh.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include <time.h>

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


static void expectABuildOf(FileItem *fileItem, FileTimestamp *contentTime) {
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(fileItem));
    expect(editorFileExists, when(path, is_equal_to_string("test.c")), will_return(true));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    expect(editorFileModificationTime, will_return(contentTime));
}

/* sideEffect, if not NULL, runs when the parse starts */
static void expectAParseWithOptionalSideEffect(void (*sideEffect)(void *)) {
    if (sideEffect != NULL)
        expect(initializeFileProcessing, with_side_effect(sideEffect, NULL), will_return(true));
    else
        expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences, when(fileName, is_equal_to_string("test.c")));
    expect(closeCharacterBuffer);
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_should_remove_old_refs_then_parses) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectABuildOf(&fileItem, &contentTime);

    /* initializeFileProcessing returns false → no parse attempt */
    expect(initializeFileProcessing, will_return(false));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_records_the_modification_time_of_what_it_parsed) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectABuildOf(&fileItem, &contentTime);

    expect(initializeFileProcessing, will_return(false));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);

    assert_that(fileTimestampsEqual(fileItem.lastParsedMtime, contentTime));
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_records_when_it_built_the_knowledge) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectABuildOf(&fileItem, &contentTime);
    expectAParseWithOptionalSideEffect(NULL);

    FileTimestamp before = fileTimestampNow();
    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);
    FileTimestamp after = fileTimestampNow();

    assert_false(fileTimestampIsLessThan(fileItem.knowledgeTime, before));
    assert_false(fileTimestampIsLessThan(after, fileItem.knowledgeTime));
}

static FileTimestamp parseStart;

/* Sleeps after recording, so a time taken after the parse started is later. */
static void recordParseStart(void *unused) {
    parseStart = fileTimestampNow();
    nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_takes_the_knowledge_time_before_parsing) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectABuildOf(&fileItem, &contentTime);

    expectAParseWithOptionalSideEffect(recordParseStart);

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);

    assert_false(fileTimestampIsLessThan(parseStart, fileItem.knowledgeTime));
}

Ensure(ReferenceRefresh, buildKnowledgeOfCU_records_no_knowledge_when_nothing_was_parsed) {
    FileItem fileItem = {.name = "test.c"};
    FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};
    expectABuildOf(&fileItem, &contentTime);
    expect(initializeFileProcessing, will_return(false));

    ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
    buildKnowledgeOfCU(42, baseArgs);

    assert_true(fileTimestampIsZero(fileItem.knowledgeTime));
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
