#include <cgreen/cgreen.h>

#include "freshness.h"

#include "filetable.h"
#include "log.h"
#include "timestamp.h"

#include "editor.mock"
#include "filetable.mock"
#include "includegraph.mock"


Describe(Freshness);
BeforeEach(Freshness) {
    log_set_level(LOG_ERROR);
}
AfterEach(Freshness) {}


#define A_C 1
#define A_H 2
#define B_C 3

static FileItem fileItem;

static void expectACUWithKnowledgeAt(FileTimestamp knowledgeTime, FileTimestamp *changeTime) {
    fileItem = (FileItem){.name = "a.c", .knowledgeTime = knowledgeTime};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("a.c")),
           will_return(changeTime));
}

static Prerequisites *prerequisitesFromGraph(IncludeGraph *graph) {
    expect(buildIncludeGraph, will_return(graph));
    return collectPrerequisites();
}

Ensure(Freshness, compilation_unit_without_knowledge_is_out_of_date) {
    FileItem fileItem = {.name = "a.c", .knowledgeTime = NULL_TIMESTAMP};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

Ensure(Freshness, compilation_unit_without_knowledge_is_out_of_date_since_it_has_none) {
    FileItem fileItem = {.name = "a.c", .knowledgeTime = NULL_TIMESTAMP};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(NULL, A_C);

    assert_that(outOfDate.reason, is_equal_to(KNOWLEDGE_MISSING));
    assert_that(outOfDate.fileNumber, is_equal_to(A_C));
}

Ensure(Freshness, compilation_unit_changed_after_its_knowledge_is_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(2000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(1000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

Ensure(Freshness, compilation_unit_changed_after_its_knowledge_is_out_of_date_since_it_changed) {
    FileTimestamp changeTime = makeFileTimestamp(2000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(1000, 0), &changeTime);

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(NULL, A_C);

    assert_that(outOfDate.reason, is_equal_to(CU_CHANGED));
    assert_that(outOfDate.fileNumber, is_equal_to(A_C));
}

Ensure(Freshness, compilation_unit_not_changed_since_its_knowledge_is_not_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

Ensure(Freshness, the_include_closure_is_taken_from_the_prerequisites_it_is_given) {
    FileTimestamp changeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);
    static int aGraph;
    IncludeGraph *graph = (IncludeGraph *)&aGraph;
    expect(collectIncludeClosure, when(graph, is_equal_to(graph)),
           when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(graph), A_C), is_false);
}

/* The filesystem stamps modification times from a coarse clock, so a write just
 * after the knowledge time can get a time a little before it (ADR-0032) */
Ensure(Freshness, compilation_unit_stamped_in_the_gap_before_its_knowledge_is_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(1999, 995000000);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

Ensure(Freshness, the_gap_is_taken_from_the_knowledge_time_not_rounded_to_a_tick) {
    FileTimestamp changeTime = makeFileTimestamp(1999, 995000000);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 1000000), &changeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

static void expectAHeaderInTheClosureOfTheCU(FileItem *headerItem) {
    static int closure[] = {A_H}; /* Read when the mock is called */
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_set_contents_of_parameter(fileNumbers, closure, sizeof(closure)),
           will_return(1));
    always_expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_H)),
                  will_return(headerItem));
}

Ensure(Freshness, compilation_unit_with_a_header_changed_after_its_knowledge_is_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C));
}

Ensure(Freshness, compilation_unit_with_a_header_changed_after_its_knowledge_is_out_of_date_since_the_header_changed) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C);

    assert_that(outOfDate.reason, is_equal_to(PREREQUISITE_CHANGED));
    assert_that(outOfDate.fileNumber, is_equal_to(A_H));
}

/* Only a change the server noticed counts, not the time on disk (ADR-0036) */
Ensure(Freshness, compilation_unit_with_a_header_not_noticed_changed_since_its_knowledge_is_not_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(1000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);
    FileTimestamp diskTime = makeFileTimestamp(3000, 0);
    always_expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
                  will_return(&diskTime));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

/* A header that is gone was noticed so when it went, and knowledge built
 * after that is current, however long the header stays gone (ADR-0036) */
Ensure(Freshness, compilation_unit_with_a_header_gone_since_before_its_knowledge_is_not_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(4000, 0), &cuChangeTime);
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);
    FileTimestamp noContentTime = NULL_TIMESTAMP;
    always_expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
                  will_return(&noContentTime));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

/* A change the server makes itself is not stamped by the coarse clock, so
 * knowledge built right after it must still be current */
Ensure(Freshness, compilation_unit_built_right_after_a_change_the_server_made_is_not_out_of_date) {
    FileItem headerItem = {.name = "a.h", .noticeTime = changeTimeNow()};
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(fileTimestampNow(), &cuChangeTime);
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

/* The closure counts what it found, also beyond what fits; what did not fit
 * could have changed */
Ensure(Freshness, compilation_unit_whose_include_closure_did_not_fit_is_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);

    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_return(1000000));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C));
}

Ensure(Freshness, compilation_unit_whose_include_closure_did_not_fit_is_out_of_date_since_it_is_too_large) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);

    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_return(1000000));

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C);

    assert_that(outOfDate.reason, is_equal_to(CLOSURE_TOO_LARGE));
    assert_that(outOfDate.fileNumber, is_equal_to(A_C));
}

/* A file that no longer exists has no modification time */
Ensure(Freshness, compilation_unit_that_no_longer_exists_is_out_of_date) {
    FileTimestamp noChangeTime = NULL_TIMESTAMP;
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &noChangeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}
