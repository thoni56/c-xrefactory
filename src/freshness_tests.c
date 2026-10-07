#include <cgreen/cgreen.h>

#include "freshness.h"

#include "filetable.h"
#include "log.h"
#include "timestamp.h"

#include "editor.mock"
#include "filetable.mock"


Describe(Freshness);
BeforeEach(Freshness) {
    log_set_level(LOG_ERROR);
}
AfterEach(Freshness) {}


#define A_C 1

static FileItem fileItem;

static void expectACUWithKnowledgeAt(FileTimestamp knowledgeTime, FileTimestamp *changeTime) {
    fileItem = (FileItem){.name = "a.c", .knowledgeTime = knowledgeTime};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));
    expect(editorFileModificationTime, when(path, is_equal_to_string("a.c")),
           will_return(changeTime));
}

Ensure(Freshness, compilation_unit_without_knowledge_is_out_of_date) {
    FileItem fileItem = {.name = "a.c", .knowledgeTime = NULL_TIMESTAMP};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(A_C)),
           will_return(&fileItem));

    assert_that(knowledgeIsOutOfDate(A_C));
}

Ensure(Freshness, compilation_unit_changed_after_its_knowledge_is_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(2000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(1000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(A_C));
}

Ensure(Freshness, compilation_unit_not_changed_since_its_knowledge_is_not_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(1000, 0);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(A_C), is_false);
}

/* The filesystem stamps modification times from a coarse clock, so a write just
 * after the knowledge time can get a time a little before it (ADR-0032) */
Ensure(Freshness, compilation_unit_stamped_in_the_gap_before_its_knowledge_is_out_of_date) {
    FileTimestamp changeTime = makeFileTimestamp(1999, 995000000);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 0), &changeTime);

    assert_that(knowledgeIsOutOfDate(A_C));
}

Ensure(Freshness, the_gap_is_taken_from_the_knowledge_time_not_rounded_to_a_tick) {
    FileTimestamp changeTime = makeFileTimestamp(1999, 995000000);
    expectACUWithKnowledgeAt(makeFileTimestamp(2000, 1000000), &changeTime);

    assert_that(knowledgeIsOutOfDate(A_C));
}
