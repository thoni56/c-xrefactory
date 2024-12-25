#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/internal/assertions_internal.h>
#include <cgreen/unit.h>
#include <cgreen/mocks.h>
#include <cjson/cJSON.h>

#include "lsp_handler.h"

#include "log.h"
#include "cjson_utils.h"

#include "lsp_sender.mock"


Describe(LspHandler);
BeforeEach(LspHandler) {
    log_set_level(LOG_ERROR);
}
AfterEach(LspHandler) {}

static cJSON *create_initialize_request() {
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(request, "id", 1);
    cJSON_AddStringToObject(request, "method", "initialize");
    return request;
}

Ensure(LspHandler, sends_correct_initialize_response) {
    const char *expected_response_text =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{}}}";
    cJSON *expected_response = cJSON_Parse(expected_response_text);
    cJSON *captured_response; // A copy created by the mock function for send_response()

    expect(send_response, will_capture_parameter(response, captured_response));

    cJSON *mock_request = create_initialize_request();

    handle_initialize(mock_request);

    char *captured_response_text = cJSON_PrintUnformatted(captured_response);
    assert_that(expected_response_text, is_equal_to_string(captured_response_text));
    assert_that(cjson_equals(expected_response, captured_response));

    cJSON_Delete(mock_request);
}
