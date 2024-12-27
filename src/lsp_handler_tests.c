#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/internal/assertions_internal.h>
#include <cgreen/unit.h>
#include <cgreen/mocks.h>
#include <cjson/cJSON.h>

#include "lsp_handler.h"

#include "log.h"
#include "json_utils.h"

#include "lsp_sender.mock"


Describe(LspHandler);
BeforeEach(LspHandler) {
    log_set_level(LOG_ERROR);
}
AfterEach(LspHandler) {}

static cJSON *create_request(double id, const char *method) {
    cJSON *request = create_lsp_message_with_id(id);
    cJSON_AddStringToObject(request, "method", method);
    return request;
}

static cJSON *create_initialize_request(void) {
    return create_request(1, "initialize");
}

static cJSON *create_code_action_request(double id, const char *uri, int line, int character) {
    // Create the root request object
    cJSON *request = create_request(id, "textDocument/codeAction");

    cJSON *params = add_json_item(request, "params");
    cJSON *textDocument = add_json_item(params, "textDocument");
    add_json_string(textDocument, "uri", uri);

    add_lsp_range(params, line, character, line, character);

    // Add context (empty for simplicity)
    add_json_item(params, "context");

    return request;
}


Ensure(LspHandler, sends_correct_initialize_response) {
    const char *expected_response_text =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{}}}";
    JSON *expected_response = parse_json(expected_response_text);
    JSON *captured_response; // A copy created by the mock function for send_response_and_delete()

    expect(send_response_and_delete, will_capture_parameter(response, captured_response));

    JSON *mock_request = create_initialize_request();

    handle_initialize(mock_request);

    // Remove capabilities from the actual response since they will change over time
    cJSON_ReplaceItemInObject(captured_response, "capabilities", cJSON_CreateObject());

    assert_that(json_equals(expected_response, captured_response));

    delete_json(mock_request);
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

    char *expected_code_action_response_as_string = print_json(expected_code_action_response);
    char *captured_code_action_response_as_string = print_json(captured_response);
    assert_that(captured_code_action_response_as_string, is_equal_to_string(expected_code_action_response_as_string));
    free(expected_code_action_response_as_string);
    free(captured_code_action_response_as_string);
    delete_json(mock_request);
}
