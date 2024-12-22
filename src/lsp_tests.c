#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "lsp.h"

#include "log.h"

#include "lsp_reader.mock"


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

Ensure(Lsp, server_reads_from_stdin) {
    const char *input =
        "Content-Length: 58\r\n\r\n"
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";

    // Create a mock input stream
    FILE *mock_input = fmemopen((void *)input, strlen(input), "r");
    assert_that(mock_input, is_not_null);  // Ensure the stream was created successfully

    // Mock lsp_reader behavior
    LspReadResult reader_result = {
        .error_code = LSP_READER_SUCCESS,
        .payload = strdup("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}")
    };
    expect(read_lsp_message, will_return(&reader_result));

    // Call the function under test with the mock input stream
    int result = lsp_server(mock_input);

    // Validate behavior
    assert_that(result, is_equal_to(0));  // Expect success
    assert_that(reader_result.payload, is_not_null);
    assert_that(reader_result.payload, is_equal_to_string("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}"));

    // Clean up
    fclose(mock_input);
}
