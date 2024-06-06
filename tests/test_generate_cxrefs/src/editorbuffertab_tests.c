#include <cgreen/cgreen.h>

#include "log.h"

#include "editorbuffertab.h"

#include "commons.mock"
#include "stackmemory.mock"


Describe(EditorBufferTab);
BeforeEach(EditorBufferTab) {
    log_set_level(LOG_ERROR);
    initEditorBufferTable();
}
AfterEach(EditorBufferTab) {}

Ensure(EditorBufferTab, returns_minus_one_for_no_more_existing) {
    assert_that(getNextExistingEditorBufferIndex(0), is_equal_to(-1));
}

Ensure(EditorBufferTab, can_return_next_existing_file_index) {
    EditorBuffer     buffer     = {.name = "item.c"};
    EditorBufferList bufferList = {.buffer = &buffer, .next = NULL};
    int              index      = addEditorBuffer(&bufferList);

    assert_that(getNextExistingEditorBufferIndex(0), is_equal_to(index));
    assert_that(getNextExistingEditorBufferIndex(index + 1), is_equal_to(-1));
}
