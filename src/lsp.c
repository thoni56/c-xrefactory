#include "lsp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lsp_dispatcher.h"
#include "lsp_reader.h"
#include "log.h"


bool want_lsp_server(int argc, char **argv) {
    for (int i=0; i<argc; i++)
        if (strcmp(argv[i], "-lsp") == 0)
            return true;
    return false;
}

int lsp_server(FILE *input_stream) {
    char buffer[8192];  // Adjust buffer size as necessary

    size_t bytes_read = fread(buffer, 1, sizeof(buffer)-1, input_stream);

    if (bytes_read == 0) {
        fprintf(stderr, "No input received\n");
        return -1;
    }
    buffer[bytes_read] = '\0';  // Ensure null termination for safety

    // Use lsp_reader to parse the input
    LspReadResult result = read_lsp_message(buffer);
    if (result.error_code != LSP_READER_SUCCESS) {
        fprintf(stderr, "Error parsing LSP message: %d\n", result.error_code);
        return -1;
    }

    log_trace("LSP: Received request: %s", result.payload);
    dispatch_lsp_message(result.payload);

    free(result.payload);
    return 0;
}
