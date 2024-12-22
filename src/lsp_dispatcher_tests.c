#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "lsp_dispatcher.h"

#include "lsp_handler.mock"


Describe(LspDispatcher);
BeforeEach(LspDispatcher) {}
AfterEach(LspDispatcher) {}

Ensure(LspDispatcher, dispatches_trivial_initialize_message) {
    const char *message = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";

    // Mock the initialize handler
    expect(handle_initialize);

    // Call the dispatcher
    int result = dispatch_lsp_message(message);

    // Validate the dispatcher called the correct handler
    assert_that(result, is_equal_to(0));
}
