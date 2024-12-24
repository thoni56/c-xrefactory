#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "lsp_dispatcher.h"

#include "lsp_errors.h"
#include "lsp_handler.mock"


Describe(LspDispatcher);
BeforeEach(LspDispatcher) {
    log_set_level(LOG_ERROR);
}
AfterEach(LspDispatcher) {}

Ensure(LspDispatcher, dispatches_trivial_initialize_request) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";
    cJSON *request = cJSON_Parse(message);

    expect(handle_initialize);

    LspReturnCode result = dispatch_lsp_request(request);

    assert_that(result, is_equal_to(LSP_RETURN_OK));
}

Ensure(LspDispatcher, dispatches_exit_request) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"exit\"}";
    cJSON *request = cJSON_Parse(message);

    expect(handle_exit);

    LspReturnCode result = dispatch_lsp_request(request);

    assert_that(result, is_equal_to(LSP_RETURN_EXIT));
}
