#include "lsp_handler.h"

#include <cjson/cJSON.h>
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

    char *response_string = cJSON_PrintUnformatted(response);

    log_trace("LSP: Sent response: '%s'", response_string);
    send_response(response_string);

    free(response_string);
}

void handle_exit(cJSON *request) {
    log_trace("LSP: Handling 'exit'");
}

void handle_method_not_found(cJSON *request) {
    log_trace("LSP: Handling method not found");
    cJSON *error_response = cJSON_CreateObject();
    cJSON_AddStringToObject(error_response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(error_response, "id", cJSON_GetObjectItem(request, "id")->valuedouble);

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", -32601);
    cJSON_AddStringToObject(error, "message", "Method not found");
    cJSON_AddItemToObject(error_response, "error", error);

    char *response = cJSON_PrintUnformatted(error_response);
    send_response(response);

    free(response);
}
