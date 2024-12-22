#include <cgreen/cgreen.h>

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
