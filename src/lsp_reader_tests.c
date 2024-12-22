#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "lsp_reader.h"


Describe(LspReader);
BeforeEach(LspReader) {}
AfterEach(LspReader) {}

Ensure(LspReader, handles_missing_content_length) {
    const char *input = "\r\n\r\n{\"jsonrpc\":\"2.0\",\"id\":1}";

    LspReadResult result = read_lsp_message(input);

    assert_that(result.error_code, is_equal_to(LSP_READER_ERROR_NO_CONTENT_LENGTH));
    assert_that(result.payload, is_null);
}

Ensure(LspReader, handles_malformed_content_length) {
    const char *input = "Content-Length: invalid\r\n\r\n{\"jsonrpc\":\"2.0\"}";

    LspReadResult result = read_lsp_message(input);

    assert_that(result.error_code, is_equal_to(LSP_READER_ERROR_MALFORMED_CONTENT_LENGTH));
    assert_that(result.payload, is_null);
}

Ensure(LspReader, handles_missing_delimiter) {
    const char *input = "Content-Length: 55\r\r\n{\"jsonrpc\":\"2.0\"}";

    LspReadResult result = read_lsp_message(input);

    assert_that(result.error_code, is_equal_to(LSP_READER_ERROR_MISSING_DELIMITER));
    assert_that(result.payload, is_null);
}

Ensure(LspReader, handles_incomplete_payload) {
    const char *input = "Content-Length: 55\r\n\r\n{\"jsonrpc\":\"2.0\"}";

    LspReadResult result = read_lsp_message(input);

    assert_that(result.error_code, is_equal_to(LSP_READER_ERROR_INCOMPLETE_PAYLOAD));
    assert_that(result.payload, is_null);
}

Ensure(LspReader, extracts_payload_from_valid_input) {
    const char *input =
        "Content-Length: 58\r\n\r\n"
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";

    LspReadResult result = read_lsp_message(input);

    assert_that(result.error_code, is_equal_to(LSP_READER_SUCCESS));
    assert_that(result.payload, is_not_null);
    assert_that(result.payload, is_equal_to_string("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}"));

    free(result.payload);
}
