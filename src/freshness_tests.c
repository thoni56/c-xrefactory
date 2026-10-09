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

static Inputs *inputsWithGraph(IncludeGraph *graph) {
    expect(buildIncludeGraph, will_return(graph));
    return collectInputs();
}

Ensure(Freshness, compilation_unit_without_knowledge_is_out_of_date) {
    FileItem fileItem = {.name = "a.c", .knowledgeTime = NULL_TIMESTAMP};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

Ensure(Freshness, compilation_unit_changed_after_its_knowledge_is_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(2000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(1000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

Ensure(Freshness, compilation_unit_not_changed_since_its_knowledge_is_not_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(inputsWithGraph(NULL), A_C), is_false);
}

Ensure(Freshness, the_include_closure_is_taken_from_the_inputs_it_is_given) {
    FileTimestamp changeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);
    static int aGraph;
    IncludeGraph *graph = (IncludeGraph *)&aGraph;
    expect(collectIncludeClosure, when(graph, is_equal_to(graph)),
           when(fileNumber, is_equal_to(A_C)), will_return(0));

    assert_that(knowledgeIsOutOfDate(inputsWithGraph(graph), A_C), is_false);
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

Ensure(Freshness, compilation_unit_with_a_header_changed_after_its_knowledge_is_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);

    int closure[] = {A_H};
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_set_contents_of_parameter(fileNumbers, closure, sizeof(closure)),
           will_return(1));
    FileItem headerItem = {.name = "a.h"};
    FileTimestamp headerChangeTime = makeFileTimestamp(3000, 0);
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_H)),
           will_return(&headerItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
           will_return(&headerChangeTime));

    assert_that(knowledgeIsOutOfDate(inputsWithGraph(NULL), A_C));
}

Ensure(Freshness, compilation_unit_with_a_header_not_changed_since_its_knowledge_is_not_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);

    int closure[] = {A_H};
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_set_contents_of_parameter(fileNumbers, closure, sizeof(closure)),
           will_return(1));
    FileItem headerItem = {.name = "a.h"};
    FileTimestamp headerChangeTime = makeFileTimestamp(1000, 0);
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_H)),
           will_return(&headerItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
           will_return(&headerChangeTime));

    assert_that(knowledgeIsOutOfDate(inputsWithGraph(NULL), A_C), is_false);
}

/* The closure counts what it found, also beyond what fits; what did not fit
 * could have changed */
Ensure(Freshness, compilation_unit_whose_include_closure_did_not_fit_is_out_of_date) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);

    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_return(1000000));

    assert_that(knowledgeIsOutOfDate(inputsWithGraph(NULL), A_C));
}

/* A file that no longer exists has no modification time */
Ensure(Freshness, compilation_unit_that_no_longer_exists_is_out_of_date) {
    FileTimestamp noChangeTime = NULL_TIMESTAMP;
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &noChangeTime);

    assert_that(knowledgeIsOutOfDate(NULL, A_C));
}

/* Each input is asked for its change time once per request, however many CUs
 * include it */
Ensure(Freshness, a_header_two_compilation_units_include_is_asked_for_its_change_time_once) {
    FileTimestamp cuChangeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &cuChangeTime);
    int closure[] = {A_H};
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(A_C)),
           will_set_contents_of_parameter(fileNumbers, closure, sizeof(closure)),
           will_return(1));
    FileItem headerItem = {.name = "a.h"};
    FileTimestamp headerChangeTime = makeFileTimestamp(1000, 0);
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_H)),
           will_return(&headerItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("a.h")),
           will_return(&headerChangeTime));

    FileItem otherCUItem = {.name = "b.c", .knowledgeTime = makeFileTimestamp(2000, 0)};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(B_C)),
           will_return(&otherCUItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("b.c")),
           will_return(&cuChangeTime));
    expect(collectIncludeClosure, when(fileNumber, is_equal_to(B_C)),
           will_set_contents_of_parameter(fileNumbers, closure, sizeof(closure)),
           will_return(1));

    Inputs *inputs = inputsWithGraph(NULL);
    assert_that(knowledgeIsOutOfDate(inputs, A_C), is_false);
    assert_that(knowledgeIsOutOfDate(inputs, B_C), is_false);
}
