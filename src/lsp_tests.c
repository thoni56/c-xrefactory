#include <cgreen/cgreen.h>

#include "lsp.h"


Describe(Lsp);
BeforeEach(Lsp) {}
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
        "Content-Length: 55\r\n\r\n"
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";

    // Redirect stdin
    FILE *temp_stdin = fmemopen((void *)input, strlen(input), "r");
    assert_that(temp_stdin, is_not_null);  // Ensure temp_stdin was created successfully

    FILE *stdin_backup = stdin;  // Backup original stdin
    stdin = temp_stdin;

    // Call the function under test
    int result = lsp_server();

    // Validate behavior
    assert_that(result, is_equal_to(0));  // For now, we expect success
    // Additional checks can go here

    // Restore stdin and clean up
    fclose(temp_stdin);
    stdin = stdin_backup;
}
