#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "lsp.h"

#include "log.h"

#include "lsp_dispatcher.mock"
#include "lsp_errors.h"


Describe(Lsp);
BeforeEach(Lsp) {
    log_set_level(LOG_ERROR);
}
AfterEach(Lsp) {}

Ensure(Lsp, returns_true_when_lsp_option_is_in_argv) {
    char *one_arg[] = {"-lsp"};
    assert_that(want_lsp_server(sizeof(one_arg)/sizeof(one_arg[0]), one_arg));
    char *three_args[] = {"program", "arg1", "-lsp"};
    assert_that(want_lsp_server(sizeof(three_args)/sizeof(three_args[0]), three_args));
}

Ensure(Lsp, returns_false_when_lsp_option_is_not_in_argv) {
    char *args[] = {"abc"};
    assert_that(!want_lsp_server(0, NULL));
    assert_that(!want_lsp_server(1, args));
}

Ensure(Lsp, will_return_when_dispatcher_returns_error) {
    const char *input =
        "Content-Length: 58\r\n\r\n"
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";

    // Create a mock input stream
    FILE *mock_stdin = fmemopen((void *)input, strlen(input), "r");

    expect(dispatch_lsp_message, will_return(LSP_RETURN_EXIT));

    int result = lsp_server(mock_stdin);

    // Returns the result code from the dispatcher
    assert_that(result, is_equal_to(LSP_RETURN_OK));

    // Clean up
    fclose(mock_stdin);
}
