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
    JSON *request = parse_json(message);

    expect(handle_initialize);

    LspReturnCode result = dispatch_lsp_message(request);

    assert_that(result, is_equal_to(LSP_RETURN_OK));
}

Ensure(LspDispatcher, dispatches_exit_request) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"exit\"}";
    JSON *request = parse_json(message);

    expect(handle_exit);

    LspReturnCode result = dispatch_lsp_message(request);

    assert_that(result, is_equal_to(LSP_RETURN_EXIT));
}

Ensure(LspDispatcher, dispatches_responds_with_not_found_for_unsupported_request) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"not-known\"}";
    JSON *request = parse_json(message);

    expect(handle_method_not_found);

    LspReturnCode result = dispatch_lsp_message(request);

    assert_that(result, is_equal_to(LSP_RETURN_ERROR_METHOD_NOT_FOUND));
}

Ensure(LspDispatcher, dispatches_does_not_respond_to_unsupported_notification) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"method\":\"not-known\"}";
    JSON *request = parse_json(message);

    never_expect(handle_method_not_found);

    LspReturnCode result = dispatch_lsp_message(request);

    assert_that(result, is_equal_to(LSP_RETURN_ERROR_METHOD_NOT_FOUND));
}

Ensure(LspDispatcher, dispatches_shutdown) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"shutdown\"}";
    JSON *request = parse_json(message);

    expect(handle_shutdown);

    LspReturnCode result = dispatch_lsp_message(request);

    assert_that(result, is_equal_to(LSP_RETURN_OK));
}
