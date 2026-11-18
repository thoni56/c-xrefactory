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

static void expectACUWithKnowledgeAt(FileTimestamp knowledgeTime) {
    fileItem = (FileItem){.name = "a.c", .knowledgeTime = knowledgeTime};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));
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

/* A CU's change is found by the removal, by its content, which takes its
 * knowledge away (ADR-0036). Its time on disk does not count here */
Ensure(Freshness, compilation_unit_with_a_time_after_its_knowledge_is_not_out_of_date_by_it) {
    expectACUWithKnowledgeAt(makeFileTimestamp(1000, 0));
    FileTimestamp diskTime = makeFileTimestamp(2000, 0);
    always_expect(editorFileModificationTime, when(path, is_equal_to_string("a.c")),
                  will_return(&diskTime));
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

Ensure(Freshness, compilation_unit_with_knowledge_and_no_changes_prerequisite_is_current) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

Ensure(Freshness, the_include_closure_is_taken_from_the_prerequisites_it_is_given) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));
    static int aGraph;
    IncludeGraph *graph = (IncludeGraph *)&aGraph;
    expect(collectIncludeClosure, when(graph, is_equal_to(graph)),
           when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(graph), A_C), is_false);
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
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C));
}

Ensure(Freshness, compilation_unit_with_a_header_changed_after_its_knowledge_is_out_of_date_since_the_header_changed) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C);

    assert_that(outOfDate.reason, is_equal_to(PREREQUISITE_CHANGED));
    assert_that(outOfDate.fileNumber, is_equal_to(A_H));
}

/* Only a change the server noticed counts, not the time on disk (ADR-0036) */
Ensure(Freshness, compilation_unit_with_a_header_not_noticed_changed_since_its_knowledge_is_not_out_of_date) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));
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
    expectACUWithKnowledgeAt(makeFileTimestamp(4000, 0));
    FileItem headerItem = {.name = "a.h", .noticeTime = makeFileTimestamp(3000, 0)};
    expectAHeaderInTheClosureOfTheCU(&headerItem);
    FileTimestamp noContentTime = NULL_TIMESTAMP;
    always_expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
                  will_return(&noContentTime));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}


/* Knowledge built right after it must still be current */
Ensure(Freshness, compilation_unit_built_right_after_a_change_the_server_made_is_not_out_of_date) {
    FileItem headerItem = {.name = "a.h", .noticeTime = changeTimeNow()};
    expectACUWithKnowledgeAt(fileTimestampNow());
    expectAHeaderInTheClosureOfTheCU(&headerItem);

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C), is_false);
}

/* The closure counts what it found, also beyond what fits; what did not fit
 * could have changed */
Ensure(Freshness, compilation_unit_whose_include_closure_did_not_fit_is_out_of_date) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));

    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_return(1000000));

    assert_that(knowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C));
}

Ensure(Freshness, compilation_unit_whose_include_closure_did_not_fit_is_out_of_date_since_it_is_too_large) {
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0));

    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_return(1000000));

    OutOfDate outOfDate = whyKnowledgeIsOutOfDate(prerequisitesFromGraph(NULL), A_C);

    assert_that(outOfDate.reason, is_equal_to(CLOSURE_TOO_LARGE));
    assert_that(outOfDate.fileNumber, is_equal_to(A_C));
}
