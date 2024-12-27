#include "lsp_sender.h"

#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "json_utils.h"
#include "log.h"
#include "lsp_errors.h"


int send_response_and_delete(JSON *response) {
    char *response_string = print_json(response);
    log_trace("LSP: Sent response: '%s'", response_string);

    printf("Content-Length: %zu\r\n\r\n%s\n", strlen(response_string), response_string);
    fflush(stdout);

    free(response_string);
    delete_json(response);

    return LSP_RETURN_OK;
}
