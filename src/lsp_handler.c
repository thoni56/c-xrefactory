#include "lsp_handler.h"

#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "lsp_sender.h"
#include "cjson_utils.h"


void handle_initialize(cJSON *request) {
    log_trace("LSP: Handling 'initialize'");

    cJSON *response = create_lsp_message_with_id(id_of_request(request));

    cJSON *result = cJSON_AddObjectToObject(response, "result");
    cJSON *capabilities = cJSON_AddObjectToObject(result, "capabilities");
    cJSON_AddBoolToObject(capabilities, "codeActionProvider", true);

    send_response_and_delete(response);
}

void handle_code_action(cJSON *request) {
    log_trace("LSP: Handling 'textDocument/codeAction'");

    cJSON *response = create_lsp_message_with_id(id_of_request(request));

    // Create the result array (list of code actions)
    cJSON *actions = cJSON_CreateArray();
    cJSON_AddItemToObject(response, "result", actions);

    cJSON *noop_action = cJSON_CreateObject();
    cJSON_AddItemToArray(actions, noop_action);
    cJSON_AddStringToObject(noop_action, "title", "Noop Action");
    cJSON_AddStringToObject(noop_action, "kind", "quickfix");

    cJSON *insert_dummy_text_action = cJSON_CreateObject();
    cJSON_AddItemToArray(actions, insert_dummy_text_action);
    cJSON_AddStringToObject(insert_dummy_text_action, "title", "Insert Dummy Text");
    cJSON_AddStringToObject(insert_dummy_text_action, "kind", "quickfix");

    cJSON *edit = cJSON_CreateObject();
    cJSON_AddItemToObject(insert_dummy_text_action, "edit", edit);

    cJSON *changes = cJSON_CreateObject();
    cJSON_AddItemToObject(edit, "changes", changes);

    cJSON *uri = cJSON_CreateArray();
    cJSON *response_uri_item = cJSON_GetObjectItem(
        cJSON_GetObjectItem(
            cJSON_GetObjectItem(request, "params"),
            "textDocument"
        ),
        "uri"
    );
    cJSON_AddItemToObject(changes, response_uri_item->valuestring, uri);
    cJSON *change = cJSON_CreateObject();
    cJSON_AddItemToArray(uri, change);

    cJSON *range = cJSON_CreateObject();
    cJSON_AddItemToObject(change, "range", range);

    cJSON *start = cJSON_CreateObject();
    cJSON_AddItemToObject(range, "start", start);
    cJSON_AddNumberToObject(start, "line", 0);
    cJSON_AddNumberToObject(start, "character", 0);

    cJSON *end = cJSON_CreateObject();
    cJSON_AddItemToObject(range, "end", end);
    cJSON_AddNumberToObject(end, "line", 0);
    cJSON_AddNumberToObject(end, "character", 0);

    cJSON_AddStringToObject(change, "newText", "Dummy Text");

    send_response_and_delete(response);
}

void handle_execute_command(cJSON *request) {
    log_trace("LSP: Handling 'workspace/executeCommand'");

    cJSON *params = cJSON_GetObjectItem(request, "params");
    const cJSON *command = cJSON_GetObjectItem(params, "command");
    const char *cmd = command->valuestring;

    if (command && strcmp(cmd, "dummyCommand") == 0) {
        log_info("LSP: Executing dummy command");
        // Perform some action or return a result
    }

    cJSON *response = cJSON_CreateNull();
    send_response_and_delete(response);
    cJSON_Delete(response);
}

void handle_shutdown(cJSON *request) {
    log_trace("LSP: Handling 'shutdown'");

    cJSON *response = create_lsp_message_with_id(id_of_request(request));
    cJSON_AddNullToObject(response, "result");

    send_response_and_delete(response);
}

void handle_exit(cJSON *request) {
    log_trace("LSP: Handling 'exit'");
}

void handle_method_not_found(cJSON *request) {
    log_trace("LSP: Handling method not found");

    cJSON *response = create_lsp_message_with_id(id_of_request(request));

    cJSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", -32601);
    cJSON_AddStringToObject(error, "message", "Method not found");
    cJSON_AddItemToObject(response, "error", error);

    send_response_and_delete(response);
}
