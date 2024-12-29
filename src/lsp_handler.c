#include "lsp_handler.h"

#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "lsp_sender.h"
#include "json_utils.h"


void handle_initialize(JSON *request) {
    log_trace("LSP: Handling 'initialize'");

    JSON *response = create_lsp_message_with_id(id_of_request(request));

    JSON *result = add_json_item(response, "result");
    JSON *capabilities = add_json_item(result, "capabilities");
    add_json_bool(capabilities, "codeActionProvider", true);

    send_response_and_delete(response);
}

/* What code actions are available at this location? */
void handle_code_action(JSON *request) {
    log_trace("LSP: Handling 'textDocument/codeAction'");

    JSON *response = create_lsp_message_with_id(id_of_request(request));

    JSON *actions = add_json_array_as(response, "result");

    add_lsp_action(actions, "Noop Action", "quickfix");

    JSON *insert_dummy_text_action = add_lsp_action(actions, "Insert Dummy Text", "quickfix");

    JSON *edit = add_json_item(insert_dummy_text_action, "edit");
    JSON *changes = add_json_item(edit, "changes");

    JSON *uri = add_json_array_as(changes, get_uri_string_from_request(request));
    JSON *change = add_json_object_to_array(uri);

    JSON *params = cJSON_GetObjectItem(request, "params");

    int start_line, start_character, end_line, end_character;
    get_lsp_range_positions(params, &start_line, &start_character, &end_line, &end_character);

    add_lsp_range(change, start_line, start_character, end_line, end_character);

    add_lsp_new_text(change, "Dummy Text");

    send_response_and_delete(response);
}

void handle_execute_command(JSON *request) {
    log_trace("LSP: Handling 'workspace/executeCommand'");

    JSON *params = cJSON_GetObjectItem(request, "params");
    const JSON *command = cJSON_GetObjectItem(params, "command");
    const char *cmd = command->valuestring;

    if (command && strcmp(cmd, "dummyCommand") == 0) {
        log_info("LSP: Executing dummy command");
        // Perform some action or return a result
    }

    JSON *response = cJSON_CreateNull();
    send_response_and_delete(response);
    delete_json(response);
}

void handle_did_open(JSON *notification) {
    log_trace("LSP: Handling 'textDocument/didOpen'");

    JSON *params = cJSON_GetObjectItem(notification, "params");

    JSON *textDocument = cJSON_GetObjectItem(params, "textDocument");
    const char *uri = cJSON_GetStringValue(cJSON_GetObjectItem(textDocument, "uri"));
    const char *languageId = cJSON_GetStringValue(cJSON_GetObjectItem(textDocument, "languageId"));
    const char *text = cJSON_GetStringValue(cJSON_GetObjectItem(textDocument, "text"));

    log_trace("LSP: Opened file '%s', language '%s', text = '%s'", uri, languageId, text);

    // TODO Ensure file is in the fileTable and put the text into an EditorBuffer

    delete_json(notification);
}

void handle_shutdown(JSON *request) {
    log_trace("LSP: Handling 'shutdown'");

    JSON *response = create_lsp_message_with_id(id_of_request(request));
    cJSON_AddNullToObject(response, "result");

    send_response_and_delete(response);
}

void handle_exit(JSON *request) {
    log_trace("LSP: Handling 'exit'");
}

void handle_method_not_found(JSON *request) {
    log_trace("LSP: Handling method not found");

    JSON *response = create_lsp_message_with_id(id_of_request(request));

    JSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", -32601);
    cJSON_AddStringToObject(error, "message", "Method not found");
    cJSON_AddItemToObject(response, "error", error);

    send_response_and_delete(response);
}
