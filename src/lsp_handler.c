#include "lsp_handler.h"

#include <stdlib.h>

#include "log.h"
#include "lsp_sender.h"


void handle_initialize(cJSON *request) {
    log_trace("LSP: Handling 'initialize'");

    // Create the response object
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", cJSON_GetObjectItem(request, "id")->valuedouble);

    cJSON *result = cJSON_AddObjectToObject(response, "result");
    cJSON_AddObjectToObject(result, "capabilities");

    // Convert response to string
    char *response_string = cJSON_PrintUnformatted(response);

    // Send the response
    log_trace("LSP: Sent response: '%s'", response_string);
    send_response(response_string);

    // Clean up
    cJSON_Delete(response);
    free(response_string);
}
