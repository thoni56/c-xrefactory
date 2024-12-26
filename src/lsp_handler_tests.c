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
    cJSON *request = create_lsp_message_with_id(1);
    cJSON_AddStringToObject(request, "method", "initialize");
    return request;
}

static cJSON *create_code_action_request(double id, const char *uri, int line, int character) {
    // Create the root request object
    cJSON *request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(request, "id", id);
    cJSON_AddStringToObject(request, "method", "textDocument/codeAction");

    // Create the params object
    cJSON *params = cJSON_CreateObject();
    cJSON *textDocument = cJSON_CreateObject();
    cJSON_AddStringToObject(textDocument, "uri", uri);
    cJSON_AddItemToObject(params, "textDocument", textDocument);

    // Add range object
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", line);
    cJSON_AddNumberToObject(start, "character", character);
    cJSON_AddItemToObject(range, "start", start);

    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", line);
    cJSON_AddNumberToObject(end, "character", character);
    cJSON_AddItemToObject(range, "end", end);

    cJSON_AddItemToObject(params, "range", range);

    // Add context (empty for simplicity)
    cJSON *context = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "context", context);

    cJSON_AddItemToObject(request, "params", params);

    return request;
}


Ensure(LspHandler, sends_correct_initialize_response) {
    const char *expected_response_text =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{}}}";
    cJSON *expected_response = cJSON_Parse(expected_response_text);
    cJSON *captured_response; // A copy created by the mock function for send_response_and_delete()

    expect(send_response_and_delete, will_capture_parameter(response, captured_response));

    cJSON *mock_request = create_initialize_request();

    handle_initialize(mock_request);

    // Remove capabilities from the actual response since they will change over time
    cJSON_ReplaceItemInObject(captured_response, "capabilities", cJSON_CreateObject());

    assert_that(cjson_equals(expected_response, captured_response));

    cJSON_Delete(mock_request);
}

Ensure(LspHandler, creates_code_action) {
    const char *editable_expected_code_action_response =
        "{"
        "\"jsonrpc\":\"2.0\","
        "\"id\":2,"
        "\"result\":["
        "    {"
        "        \"title\":\"Noop Action\","
        "        \"kind\":\"quickfix\""
        "    },"
        "    {"
        "        \"title\":\"Insert Dummy Text\","
        "        \"kind\":\"quickfix\","
        "        \"edit\":{"
        "            \"changes\":{"
        "                \"file:///path/to/file.c\":["
        "                    {"
        "                        \"range\":{"
        "                            \"start\":{\"line\":0,\"character\":0},"
        "                            \"end\":{\"line\":0,\"character\":0}"
        "                        },"
        "                        \"newText\":\"Dummy Text\""
        "                    }"
        "                ]"
        "            }"
        "        }"
        "    }"
        "]"
        "}";
    cJSON *expected_code_action_response = cJSON_Parse(editable_expected_code_action_response);

    cJSON *captured_response; // A copy created by the mock function for send_response_and_delete()
    expect(send_response_and_delete, will_capture_parameter(response, captured_response));

    cJSON *mock_request = create_code_action_request(2, "file:///path/to/file.c", 1, 0);

    handle_code_action(mock_request);

    const char *expected_code_action_response_as_string = cJSON_PrintUnformatted(expected_code_action_response);
    const char *captured_code_action_response_as_string = cJSON_PrintUnformatted(captured_response);
    assert_that(captured_code_action_response_as_string, is_equal_to_string(expected_code_action_response_as_string));

    cJSON_Delete(mock_request);

}
