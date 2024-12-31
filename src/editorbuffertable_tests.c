#include <cgreen/cgreen.h>

#include "editorbuffer.h"
#include "log.h"

#include "editorbuffertable.h"
#include "hash.h"

#include "commons.mock"
#include "stackmemory.mock"


extern EditorBufferTable editorBufferTable;


Describe(EditorBufferTable);
BeforeEach(EditorBufferTable) {
    log_set_level(LOG_ERROR);
    initEditorBufferTable();
}
AfterEach(EditorBufferTable) {}

static EditorBufferList *createEditorBufferListElementFor(char *fileName) {
    /* All parts need to be malloc'd so that they can be freed */
    EditorBuffer     *buffer     = malloc(sizeof(EditorBuffer));
    *buffer = (EditorBuffer){.fileName = strdup(fileName)};
    EditorBufferList *bufferList = malloc(sizeof(EditorBufferList));
    *bufferList = (EditorBufferList){.buffer = buffer, .next = NULL};
    return bufferList;
}

Ensure(EditorBufferTable, returns_minus_one_for_no_more_existing) {
    assert_that(getNextExistingEditorBufferIndex(0), is_equal_to(-1));
}

Ensure(EditorBufferTable, can_return_next_existing_file_index) {
    char *fileName = "item.c";
    EditorBufferList *bufferList = createEditorBufferListElementFor(fileName);
    int index = addEditorBuffer(bufferList);
    assert_that(editorBufferTable.tab[index], is_equal_to(bufferList));

    assert_that(getNextExistingEditorBufferIndex(0), is_equal_to(index));
    assert_that(getNextExistingEditorBufferIndex(index + 1), is_equal_to(-1));
}

Ensure(EditorBufferTable, can_add_one) {
    char *fileName = "item.c";
    EditorBufferList *bufferList = createEditorBufferListElementFor(fileName);
    int index = addEditorBuffer(bufferList);

    assert_that(editorBufferTable.tab[index], is_equal_to(bufferList));
}

Ensure(EditorBufferTable, will_not_add_editorbuffer_for_the_same_filename_twice) {
    char *fileName = "item.c";
    EditorBufferList *bufferList1 = createEditorBufferListElementFor(fileName);
    int index1 = addEditorBuffer(bufferList1);

    EditorBufferList *bufferList2 = createEditorBufferListElementFor(fileName);
    int index2 = addEditorBuffer(bufferList2);

    assert_that(index1, is_equal_to(index2));
    assert_that(editorBufferTable.tab[index1], is_equal_to(bufferList1));
    assert_that(editorBufferTable.tab[index1]->next, is_null);
}


unsigned injectedHashFun(char *fileName) {
    return (unsigned)mock(fileName);
}

Ensure(EditorBufferTable, can_add_editorbuffer_for_filename_with_same_hash) {
    char *fileName = "item.c";
    EditorBufferList *bufferList1 = createEditorBufferListElementFor(fileName);
    int index1 = addEditorBuffer(bufferList1);

    /* Inject a hashFun that we can control */
    injectHashFun(injectedHashFun);
    /* ... and let it return the same hash as for the first file name */
    expect(injectedHashFun, will_return(hashFun(fileName)));

    EditorBufferList *bufferList2 = createEditorBufferListElementFor("matchingHash.c");
    int index2 = addEditorBuffer(bufferList2);

    assert_that(index1, is_equal_to(index2));
    assert_that(editorBufferTable.tab[index1], is_equal_to(bufferList2));
    assert_that(editorBufferTable.tab[index1]->next, is_equal_to(bufferList1));
}

Ensure(EditorBufferTable, can_register_and_deregister_a_buffer) {
    EditorBuffer *registered_buffer = malloc(sizeof(EditorBuffer));
    *registered_buffer = (EditorBuffer){.fileName = strdup("file.c")};

    int registered_index = registerEditorBuffer(registered_buffer);

    assert_that(editorBufferTable.tab[registered_index]->buffer, is_equal_to(registered_buffer));
}
