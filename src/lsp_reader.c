#include "lsp_reader.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


LspReadResult read_lsp_message(const char *input) {
    LspReadResult result = {.payload = NULL, .error_code = LSP_READER_SUCCESS };

    // Find "Content-Length" header
    const char *content_length_start = strstr(input, "Content-Length:");
    if (!content_length_start) {
        result.error_code = LSP_READER_ERROR_NO_CONTENT_LENGTH;
        return result;
    }

    // Parse content length
    size_t content_length;
    if (sscanf(content_length_start, "Content-Length: %zu", &content_length) != 1) {
        result.error_code = LSP_READER_ERROR_MALFORMED_CONTENT_LENGTH;
        return result;
    }

    // Find the end of the headers
    const char *payload_start = strstr(content_length_start, "\r\n\r\n");
    if (!payload_start) {
        result.error_code = LSP_READER_ERROR_MISSING_DELIMITER;
        return result;
    }

    payload_start += 4;  // Skip "\r\n\r\n"

    // Ensure the payload length matches Content-Length
    if (strlen(payload_start) < content_length) {
        result.error_code = LSP_READER_ERROR_INCOMPLETE_PAYLOAD;
        return result;
    }

    // Copy the payload
    result.payload = strndup(payload_start, content_length);
    if (!result.payload) {
        result.error_code = LSP_READER_ERROR_INCOMPLETE_PAYLOAD;
    }

    return result;
}
