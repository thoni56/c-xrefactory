#ifndef LSP_READER_H_INCLUDED
#define LSP_READER_H_INCLUDED

#include <stddef.h>

typedef enum {
    LSP_READER_SUCCESS,
    LSP_READER_ERROR_NO_CONTENT_LENGTH,
    LSP_READER_ERROR_MISSING_DELIMITER,
    LSP_READER_ERROR_MALFORMED_CONTENT_LENGTH,
    LSP_READER_ERROR_INCOMPLETE_PAYLOAD
} LspReadStatus;

typedef struct {
    LspReadStatus error_code; // 0 for success, non-zero for specific errors
    char *payload;            // Pointer to the parsed JSON payload
} LspReadResult;


/**
 * Parses the LSP input string, extracting the payload.
 *
 * @param input The input string containing headers and payload.
 * @return LspReadResult with an error code and possibly a malloc'ed copy of the payload.
 */
LspReadResult read_lsp_message(const char *input);

#endif
