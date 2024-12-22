#ifndef LSP_READER_H_INCLUDED
#define LSP_READER_H_INCLUDED

#include <stddef.h>

typedef enum {
    LSP_READER_SUCCESS,
    LSP_READER_ERROR_NO_CONTENT_LENGTH,
    LSP_READER_ERROR_MALFORMED_CONTENT_LENGTH,
    LSP_READER_ERROR_INCOMPLETE_PAYLOAD
} LspReadStatus;

typedef struct {
    char *payload;  // Pointer to the parsed JSON payload
    LspReadStatus error_code; // 0 for success, non-zero for specific errors
} LspReadResult;


/**
 * Parses the LSP input string, extracting the payload.
 *
 * @param input The input string containing headers and payload.
 * @return LspReadResult with the payload and error code.
 */
LspReadResult read_lsp_message(const char *input);

#endif
