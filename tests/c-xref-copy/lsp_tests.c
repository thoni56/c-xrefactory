#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "argumentsvector.h"
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
    ArgumentsVector oneArgs = {.argc = sizeof(one_arg)/sizeof(one_arg[0]), one_arg};
    assert_that(want_lsp_server(oneArgs));

    char *three_args[] = {"program", "arg1", "-lsp"};
    ArgumentsVector args = {.argc = sizeof(three_args)/sizeof(three_args[0]), three_args};
    assert_that(want_lsp_server(args));
}

Ensure(Lsp, returns_false_when_lsp_option_is_not_in_argv) {
    char *arguments[] = {"abc"};
    ArgumentsVector args = {.argc = 0, .argv = NULL};
    assert_that(!want_lsp_server(args));

    args = (ArgumentsVector){.argc = 1, .argv = arguments};
    assert_that(!want_lsp_server(args));
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
