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

static FileTimestamp seenTime = {.tv_sec = 1000, .tv_nsec = 0};
static FileTimestamp newerTime = {.tv_sec = 2000, .tv_nsec = 0};

static void expectOneHeaderInTheFileTable(FileItem *fileItem, FileTimestamp *contentTime) {
    expect(getNextExistingFileNumber, will_return(42));
    expect(getNextExistingFileNumber, will_return(-1));
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(fileItem));
    always_expect(isCompilationUnit, will_return(false));
    expect(editorFileModificationTime, when(path, is_equal_to_string(fileItem->name)),
           will_return(contentTime));
}

Ensure(Rebuild, removes_the_references_of_a_header_whose_content_changed) {
    FileItem fileItem = {.name = "header.h", .changeTime = seenTime};
    expectOneHeaderInTheFileTable(&fileItem, &newerTime);
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));

    removeReferencesOfChangedFiles(NULL);

    assert_that(fileTimestampsEqual(fileItem.changeTime, newerTime));
}

Ensure(Rebuild, keeps_the_references_of_a_header_whose_content_did_not_change) {
    FileItem fileItem = {.name = "header.h", .changeTime = seenTime};
    expectOneHeaderInTheFileTable(&fileItem, &seenTime);
    never_expect(removeReferenceableItemsForFile);

    removeReferencesOfChangedFiles(NULL);
}

Ensure(Rebuild, records_the_change_time_of_a_header_seen_for_the_first_time_and_keeps_its_references) {
    FileItem fileItem = {.name = "header.h"};
    expectOneHeaderInTheFileTable(&fileItem, &seenTime);
    never_expect(removeReferenceableItemsForFile);

    removeReferencesOfChangedFiles(NULL);

    assert_that(fileTimestampsEqual(fileItem.changeTime, seenTime));
}

static void expectTwoHeadersInTheFileTable(FileItem *leaf, FileTimestamp *leafTime,
                                           FileItem *other, FileTimestamp *otherTime) {
    expect(getNextExistingFileNumber, will_return(42));
    expect(getNextExistingFileNumber, will_return(43));
    expect(getNextExistingFileNumber, will_return(-1));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(leaf));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(43)), will_return(other));
    always_expect(isCompilationUnit, will_return(false));
    expect(editorFileModificationTime, will_return(leafTime));
    expect(editorFileModificationTime, will_return(otherTime));
}

/* What a header emits depends on what it includes (ADR-0035) */
Ensure(Rebuild, removes_the_references_of_a_header_that_includes_a_changed_one) {
    FileItem leaf = {.name = "leaf.h", .changeTime = seenTime};
    FileItem mid = {.name = "mid.h", .changeTime = seenTime};
    expectTwoHeadersInTheFileTable(&leaf, &newerTime, &mid, &seenTime);
    static int closureOfMid[] = {42};
    expect(collectIncludeClosureOf, when(fileNumber, is_equal_to(43)),
           will_set_contents_of_parameter(fileNumbers, closureOfMid, sizeof(closureOfMid)),
           will_return(1));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(43)));

    removeReferencesOfChangedFiles(NULL);

    assert_that(fileTimestampsEqual(mid.changeTime, newerTime));
}

Ensure(Rebuild, keeps_the_references_of_a_header_that_does_not_include_a_changed_one) {
    FileItem leaf = {.name = "leaf.h", .changeTime = seenTime};
    FileItem other = {.name = "other.h", .changeTime = seenTime};
    expectTwoHeadersInTheFileTable(&leaf, &newerTime, &other, &seenTime);
    expect(collectIncludeClosureOf, when(fileNumber, is_equal_to(43)), will_return(0));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));

    removeReferencesOfChangedFiles(NULL);

    assert_that(fileTimestampsEqual(other.changeTime, seenTime));
}

Ensure(Rebuild, letting_go_of_a_held_header_removes_its_references) {
    FileItem fileItem = {.name = "header.h"};
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(&fileItem));
    expect(removeReferenceableItemsForFile, when(fileNumber, is_equal_to(42)));
    always_expect(changeTimeNow, will_return(&newerTime));

    letGoOfHeldHeader(42);
}

/* The disk is older than the knowledge built from the buffer, so the change is now */
Ensure(Rebuild, letting_go_of_a_held_header_changes_it_now) {
    FileItem fileItem = {.name = "header.h", .changeTime = seenTime};
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)), will_return(&fileItem));
    always_expect(removeReferenceableItemsForFile);
    expect(changeTimeNow, will_return(&newerTime));

    letGoOfHeldHeader(42);

    assert_that(fileTimestampsEqual(fileItem.changeTime, newerTime));
}
