#include "rebuild.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include <time.h>

#include "filetable.h"
#include "log.h"
#include "options.h"

#include "characterreader.mock"
#include "commons.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "editorbuffertable.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "freshness.mock"
#include "globals.mock"
#include "misc.mock"
#include "navigation.mock"
#include "options.mock"
#include "parsing.mock"
#include "progress.mock"
#include "referenceableitemtable.mock"
#include "startup.mock"


Describe(Rebuild);
BeforeEach(Rebuild) {
    log_set_level(LOG_ERROR);
    always_expect(silenceMessages);
    always_expect(resumeMessages);
    options.cursorOffset = NO_CURSOR_OFFSET;
}
AfterEach(Rebuild) {}


static ArgumentsVector baseArgs = {.argc = 0, .argv = NULL};
static FileTimestamp contentTime = {.tv_sec = 1234, .tv_nsec = 5678};

static BuildList *buildListOf(int fileNumber) {
    BuildList *list = newBuildList(MAX_CUS_TO_REPARSE);
    addToBuildList(list, fileNumber);
    return list;
}

static void expectAFileOnDisk(FileItem *fileItem) {
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(fileItem));
    expect(editorFileExists, when(path, is_equal_to_string(fileItem->name)), will_return(true));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    expect(editorFileModificationTime, will_return(&contentTime));
}

/* sideEffect, if not NULL, runs when the parse starts */
static void expectAParseWithOptionalSideEffect(char *fileName, void (*sideEffect)(void *)) {
    if (sideEffect != NULL)
        expect(initializeFileProcessing, with_side_effect(sideEffect, NULL), will_return(true));
    else
        expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences, when(fileName, is_equal_to_string(fileName)));
    expect(closeCharacterBuffer);
}


Ensure(Rebuild, builds_every_cu_on_the_list_in_its_order) {
    FileItem fileItem = {.name = "test.c"};
    always_expect(getFileItemWithFileNumber, will_return(&fileItem));
    always_expect(editorFileExists, will_return(true));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(1)));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(2)));
    always_expect(editorFileModificationTime, will_return(&contentTime));
    expectAParseWithOptionalSideEffect("test.c", NULL);
    expectAParseWithOptionalSideEffect("test.c", NULL);

    BuildList *list = newBuildList(MAX_CUS_TO_REPARSE);
    addToBuildList(list, 1);
    addToBuildList(list, 2);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);
}

static bool referencesRemoved;
static bool referencesRemovedBeforeTheParse;

static void recordReferencesRemoved(void *unused) {
    referencesRemoved = true;
}

static void recordWhetherReferencesWereRemoved(void *unused) {
    referencesRemovedBeforeTheParse = referencesRemoved;
}

Ensure(Rebuild, removes_the_old_references_of_a_cu_before_it_is_parsed) {
    FileItem fileItem = {.name = "test.c"};
    referencesRemoved = false;
    always_expect(getFileItemWithFileNumber, will_return(&fileItem));
    expect(editorFileExists, will_return(true));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)),
           with_side_effect(recordReferencesRemoved, NULL));
    expectAParseWithOptionalSideEffect("test.c", recordWhetherReferencesWereRemoved);
    expect(editorFileModificationTime, will_return(&contentTime));

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_that(referencesRemovedBeforeTheParse);
}

static FileTimestamp parseStart;

/* Sleeps after recording, so a time taken after the parse started is later. */
static void recordParseStart(void *unused) {
    parseStart = fileTimestampNow();
    nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
}

Ensure(Rebuild, a_built_cu_knows_it_was_built_from_what_it_had_when_the_parse_started) {
    FileItem fileItem = {.name = "test.c"};
    expectAFileOnDisk(&fileItem);
    expectAParseWithOptionalSideEffect("test.c", recordParseStart);

    FileTimestamp before = fileTimestampNow();
    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_false(fileTimestampIsLessThan(fileItem.knowledgeTime, before));
    assert_false(fileTimestampIsLessThan(parseStart, fileItem.knowledgeTime));
}

Ensure(Rebuild, a_built_cu_records_the_modification_time_it_was_built_from) {
    FileItem fileItem = {.name = "test.c"};
    expectAFileOnDisk(&fileItem);
    expectAParseWithOptionalSideEffect("test.c", NULL);

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_that(fileTimestampsEqual(fileItem.lastParsedMtime, contentTime));
}

Ensure(Rebuild, a_cu_that_did_not_parse_has_no_knowledge) {
    FileItem fileItem = {.name = "test.c"};
    expectAFileOnDisk(&fileItem);
    expect(initializeFileProcessing, will_return(false));
    never_expect(parseToCreateReferences);

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_that(fileTimestampIsZero(fileItem.knowledgeTime));
}

Ensure(Rebuild, a_cu_gone_from_disk_is_forgotten_not_parsed) {
    FileItem fileItem = {.name = "test.c"};
    always_expect(getFileItemWithFileNumber, will_return(&fileItem));
    expect(editorFileExists, will_return(false));
    expect(markFileAsDeleted, when(fileNumber, is_equal_to(42)));
    never_expect(initializeFileProcessing);
    never_expect(parseToCreateReferences);

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);
}

static int cursorOffsetDuringTheParse;

static void recordCursorOffset(void *unused) {
    cursorOffsetDuringTheParse = options.cursorOffset;
}

Ensure(Rebuild, does_not_see_the_cursor_of_the_request) {
    FileItem fileItem = {.name = "test.c"};
    expectAFileOnDisk(&fileItem);
    expect(initializeFileProcessing, will_return(true));
    expect(parseToCreateReferences, with_side_effect(recordCursorOffset, NULL));
    expect(closeCharacterBuffer);
    options.cursorOffset = 17;

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_that(cursorOffsetDuringTheParse, is_equal_to(NO_CURSOR_OFFSET));
    assert_that(options.cursorOffset, is_equal_to(17));
}

Ensure(Rebuild, a_built_cu_has_its_browsing_stack_refreshed) {
    FileItem fileItem = {.name = "test.c"};
    expectAFileOnDisk(&fileItem);
    expectAParseWithOptionalSideEffect("test.c", NULL);

    BuildList *list = buildListOf(42);
    buildKnowledgeOfAll(list, baseArgs);
    freeBuildList(list);

    assert_that(fileItem.needsBrowsingStackRefresh);
}
