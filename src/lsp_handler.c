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

    cJSON *result = add_item(response, "result");
    cJSON *capabilities = add_item(result, "capabilities");
    add_bool(capabilities, "codeActionProvider", true);

    send_response_and_delete(response);
}

/* What code actions are available at this location? */
void handle_code_action(cJSON *request) {
    log_trace("LSP: Handling 'textDocument/codeAction'");

    cJSON *response = create_lsp_message_with_id(id_of_request(request));

    cJSON *actions = add_array_as(response, "result");

    add_action(actions, "Noop Action", "quickfix");

    cJSON *insert_dummy_text_action = add_action(actions, "Insert Dummy Text", "quickfix");

    cJSON *edit = add_item(insert_dummy_text_action, "edit");
    cJSON *changes = add_item(edit, "changes");

    cJSON *uri = add_array_as(changes, get_uri_string_from_request(request));
    cJSON *change = add_object_to_array(uri);

    add_range(change, 0, 0, 0, 0);

    add_new_text(change, "Dummy Text");

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
