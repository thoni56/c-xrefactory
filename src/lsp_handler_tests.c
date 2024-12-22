#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/unit.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "lsp_handler.h"

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
    const char *expected_response =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"capabilities\":{}}}";

    // Expect the output function
    expect(send_response, when(response, is_equal_to_string(expected_response)));

    // Create a minimal cJSON object for the initialize request
    cJSON *mock_request = create_initialize_request();

    // Call the handler
    handle_initialize(mock_request);

    // Clean up
    cJSON_Delete(mock_request);
}
