#include "lsp_handler.h"

#include <cjson/cJSON.h>
#include <stdlib.h>

#include "log.h"
#include "lsp_sender.h"

static cJSON *create_response(double id) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);
    return response;
}

static double id_of(cJSON *request) {
    return cJSON_GetObjectItem(request, "id")->valuedouble;
}

void handle_initialize(cJSON *request) {
    log_trace("LSP: Handling 'initialize'");

    cJSON *response = create_response(id_of(request));

    cJSON *result = cJSON_AddObjectToObject(response, "result");
    cJSON_AddObjectToObject(result, "capabilities");

    send_response(response);
    cJSON_Delete(response);
}

void handle_shutdown(cJSON *request) {
    log_trace("LSP: Handling 'shutdown'");

    cJSON *response = create_response(id_of(request));
    cJSON_AddNullToObject(response, "result");

    send_response(response);
    cJSON_Delete(response);
}

void handle_exit(cJSON *request) {
    log_trace("LSP: Handling 'exit'");
}

void handle_method_not_found(cJSON *request) {
    log_trace("LSP: Handling method not found");

    cJSON *response = create_response(id_of(request));

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", -32601);
    cJSON_AddStringToObject(error, "message", "Method not found");
    cJSON_AddItemToObject(response, "error", error);

    send_response(response);
    cJSON_Delete(response);
}
