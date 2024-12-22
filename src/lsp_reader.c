#include "lsp_reader.h"

#include <string.h>


LspReadResult read_lsp_message(const char *input) {
    LspReadResult result = {.payload = NULL, .error_code = LSP_READER_SUCCESS };

    // Find "Content-Length" header
    const char *content_length_start = strstr(input, "Content-Length:");
    if (!content_length_start) {
        result.error_code = LSP_READER_ERROR_NO_CONTENT_LENGTH;
        return result;
    }

    return result;
}
