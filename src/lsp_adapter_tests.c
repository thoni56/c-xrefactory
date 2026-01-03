#include "lsp_adapter.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>
#include <cjson/cJSON.h>

#include "log.h"

#include "commons.mock"
#include "filetable.mock"
#include "lsp_handler.mock"
#include "referenceableitem.h"
#include "reference_database.mock"


Describe(LspAdapter);
BeforeEach(LspAdapter) {
    log_set_level(LOG_ERROR);
    /* Inject a fake database pointer for testing */
    ReferenceDatabase *mockDb = (ReferenceDatabase *)0x1234;
    setReferenceDatabase(mockDb);
}
AfterEach(LspAdapter) {
    setReferenceDatabase(NULL);
}


Ensure(LspAdapter, findDefinition_returns_location) {
    char *fileName = "/home/thoni/Utveckling/c-xrefactory/tests/test_lsp_goto_definition/source.c";
    const char *uri = "file:///home/thoni/Utveckling/c-xrefactory/tests/test_lsp_goto_definition/source.c";

    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "line", 5);
    cJSON_AddNumberToObject(position, "character", 20);

    /* Mock the findReferenceableAt call */
    ReferenceableItem referenceable = {.linkName = "symbol" };
    ReferenceableResult mockResult = {.referenceable = &referenceable,
        .definition = {.file = 42, .line = 2, .col = 6},
        .found = true};
    expect(findReferenceableAt, will_return_by_value(mockResult, sizeof(ReferenceableResult)));

    FileItem mockedFileItem = {.name = fileName};
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(42)),
           will_return_by_value(mockedFileItem, sizeof(mockedFileItem)));

    expect(getFileNumberFromFileName, will_return(42));

    cJSON *result = findDefinition(uri, position);

    assert_that(result, is_not_null);
    assert_that(cJSON_GetObjectItem(result, "uri"), is_not_null);
    assert_that(cJSON_GetObjectItem(result, "range"), is_not_null);

    cJSON *range = cJSON_GetObjectItem(result, "range");
    cJSON *start = cJSON_GetObjectItem(range, "start");
    cJSON *end = cJSON_GetObjectItem(range, "end");

    assert_that(start, is_not_null);
    assert_that(end, is_not_null);
    assert_that(cJSON_GetObjectItem(start, "line"), is_not_null);
    assert_that(cJSON_GetObjectItem(start, "character"), is_not_null);

    cJSON_Delete(position);
    cJSON_Delete(result);
}
