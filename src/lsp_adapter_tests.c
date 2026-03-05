#include "lsp_adapter.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "cJSON/cJSON.h"

#include "log.h"
#include "referenceableitem.h"
#include "usage.h"

#include "commons.mock"
#include "filetable.mock"

/* We need a custom mock for referenceableitemtable that actually invokes
 * the callback with our test data */
#include "referenceableitemtable.h"

ReferenceableItemTable referenceableItemTable;

/* Test fixture: the item that mapOver will yield to the callback */
static ReferenceableItem *testItem = NULL;

void mapOverReferenceableItemTableWithPointer(void (*fun)(ReferenceableItem *, void *), void *pointer) {
    if (testItem != NULL)
        fun(testItem, pointer);
}

/* Stubs for other referenceableitemtable functions (not used but needed by linker) */
void initReferenceableItemTable(int size) { (void)size; }


Describe(LspAdapter);
BeforeEach(LspAdapter) {
    log_set_level(LOG_ERROR);
    testItem = NULL;
}
AfterEach(LspAdapter) {}


Ensure(LspAdapter, findDefinition_returns_null_when_no_referenceable_at_position) {
    const char *uri = "file:///path/to/file.c";

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 5);
    cJSON_AddNumberToObject(position, "character", 20);

    expect(getFileNumberFromFileName, will_return(1));

    /* No testItem set — mapOver yields nothing */

    cJSON *result = findDefinition(uri, position);

    assert_that(result, is_null);

    cJSON_Delete(position);
}

Ensure(LspAdapter, findDefinition_returns_location) {
    char *fileName = "/home/thoni/Utveckling/c-xrefactory/tests/test_lsp_goto_definition/source.c";
    const char *uri = "file:///home/thoni/Utveckling/c-xrefactory/tests/test_lsp_goto_definition/source.c";

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 5);
    cJSON_AddNumberToObject(position, "character", 20);

    /* Set up a referenceable item with a reference at the queried position
     * and a definition at a known location */
    Reference defRef = {
        .position = {.file = 42, .line = 2, .col = 6},
        .usage = UsageDefined,
        .next = NULL
    };
    Reference useRef = {
        .position = {.file = 1, .line = 6, .col = 20},  /* line 6 = lsp line 5 + 1 */
        .usage = UsageUsed,
        .next = &defRef
    };
    ReferenceableItem item = {
        .linkName = "symbol",
        .references = &useRef
    };
    testItem = &item;

    expect(getFileNumberFromFileName, will_return(1));

    FileItem mockedFileItem = {.name = fileName};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return_by_value(mockedFileItem, sizeof(mockedFileItem)));

    cJSON *result = findDefinition(uri, position);

    assert_that(result, is_not_null);
    assert_that(cJSON_GetObjectItem(result, "uri"), is_not_null);
    assert_that(cJSON_GetObjectItem(result, "range"), is_not_null);

    cJSON *range = cJSON_GetObjectItem(result, "range");
    cJSON *start = cJSON_GetObjectItem(range, "start");
    cJSON *end = cJSON_GetObjectItem(range, "end");

    assert_that(start, is_not_null);
    assert_that(end, is_not_null);

    /* Definition is at line 2, col 6 in c-xrefactory coords → LSP line 1, col 6 */
    assert_that(cJSON_GetObjectItem(start, "line")->valueint, is_equal_to(1));
    assert_that(cJSON_GetObjectItem(start, "character")->valueint, is_equal_to(6));
    /* End col = 6 + strlen("symbol") = 12 */
    assert_that(cJSON_GetObjectItem(end, "character")->valueint, is_equal_to(12));

    cJSON_Delete(position);
    cJSON_Delete(result);
}
