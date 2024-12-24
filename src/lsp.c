#include "lsp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>
#include "lsp_dispatcher.h"
#include "log.h"

#define MAX_HEADER_FIELD_LEN 100

enum {
    ERROR_OK,
    ERROR_HEADER_INCOMPLETE,
    ERROR_OUT_OF_MEMORY,
    ERROR_IO_ERROR,
    ERROR_PARSE_ERROR
};

bool want_lsp_server(int argc, char **argv) {
    for (int i=0; i<argc; i++)
        if (strcmp(argv[i], "-lsp") == 0)
            return true;
    return false;
}

static unsigned long lsp_parse_header(FILE *input_stream) {
    char buffer[MAX_HEADER_FIELD_LEN];
    unsigned long content_length = 0;

    for(;;) {
        fgets(buffer, MAX_HEADER_FIELD_LEN, input_stream);
        if(strcmp(buffer, "\r\n") == 0) { // End of header
            if(content_length == 0)
                exit(ERROR_HEADER_INCOMPLETE);
            return content_length;
        }

        char *buffer_part = strtok(buffer, " ");
        if(strcmp(buffer_part, "Content-Length:") == 0) {
            buffer_part = strtok(NULL, "\n");
            content_length = atoi(buffer_part);
        }
    }
}

static cJSON* lsp_parse_content(FILE *input_stream, unsigned long content_length) {
    char *buffer = malloc(content_length + 1);
    if(buffer == NULL)
        exit(ERROR_OUT_OF_MEMORY);
    size_t read_elements = fread(buffer, 1, content_length, input_stream);
    if(read_elements != content_length) {
        free(buffer);
        exit(ERROR_IO_ERROR);
    }
    buffer[content_length] = '\0';

    cJSON *request = cJSON_Parse(buffer);

    free(buffer);
    if(request == NULL)
        exit(ERROR_PARSE_ERROR);
    return request;
}

int lsp_server(FILE *input_stream) {
    log_trace("LSP: Server started");

    int result = ERROR_OK;
    while (result == ERROR_OK) {
        unsigned long content_length = lsp_parse_header(input_stream);
        cJSON *request = lsp_parse_content(input_stream, content_length);
        result = dispatch_lsp_request(request);
        cJSON_Delete(request);
    }
    return result;
}
