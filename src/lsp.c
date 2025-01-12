#include "lsp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "json_utils.h"
#include "lsp_dispatcher.h"
#include "lsp_errors.h"
#include "log.h"


#define MAX_HEADER_FIELD_LEN 100


bool want_lsp_server(int argc, char **argv) {
    for (int i=0; i<argc; i++)
        if (strcmp(argv[i], "-lsp") == 0)
            return true;
    return false;
}

static unsigned long wait_for_and_parse_lsp_header(FILE *input_stream) {
    char buffer[MAX_HEADER_FIELD_LEN];
    unsigned long content_length = 0;

    for(;;) {
        fgets(buffer, MAX_HEADER_FIELD_LEN, input_stream);
        if (strcmp(buffer, "\r\n") == 0) { // End of header
            if (content_length == 0)
                exit(LSP_RETURN_ERROR_HEADER_INCOMPLETE);
            return content_length;
        }

        char *buffer_part = strtok(buffer, " ");
        if (strcmp(buffer_part, "Content-Length:") == 0) {
            buffer_part = strtok(NULL, "\n");
            content_length = atoi(buffer_part);
        }
    }
}

static JSON* lsp_parse_content(FILE *input_stream, unsigned long content_length) {
    char *buffer = malloc(content_length + 1);
    if (buffer == NULL)
        exit(LSP_RETURN_ERROR_OUT_OF_MEMORY);
    size_t read_elements = fread(buffer, 1, content_length, input_stream);
    if (read_elements != content_length) {
        free(buffer);
        exit(LSP_RETURN_ERROR_IO_ERROR);
    }
    buffer[content_length] = '\0';

    JSON *request = parse_json(buffer);

    free(buffer);
    if (request == NULL)
        exit(LSP_RETURN_ERROR_JSON_PARSE_ERROR);
    return request;
}

int lsp_server(FILE *input_stream) {
    log_trace("LSP: Server started");

    int result = LSP_RETURN_OK;
    while (result == LSP_RETURN_OK) {
        unsigned long content_length = wait_for_and_parse_lsp_header(input_stream);
        JSON *message = lsp_parse_content(input_stream, content_length);

        result = dispatch_lsp_message(message);

        delete_json(message);
        if (result == LSP_RETURN_ERROR_METHOD_NOT_FOUND)
            result = LSP_RETURN_OK;
    }
    if (result == LSP_RETURN_EXIT)
        return LSP_RETURN_OK;
    else
        return result;
}
