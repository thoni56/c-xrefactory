#include "lsp_handler.h"

#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>

#include "filetable.h"
#include "log.h"
#include "lsp_sender.h"
#include "json_utils.h"


static char *filename_from_uri(const char *uri) {
    char *uri_prefix = "file://";
    if (strncmp(uri, uri_prefix, strlen(uri_prefix)) == 0)
        return (char *)&uri[strlen(uri_prefix)];
    else
        return NULL;
}

void handle_initialize(JSON *request) {
    log_trace("LSP: Handling 'initialize'");

    JSON *response = create_lsp_message_with_id(id_of_request(request));

    JSON *result = add_json_item(response, "result");
    JSON *capabilities = add_json_item(result, "capabilities");
    add_json_bool(capabilities, "codeActionProvider", true);

    initFileTable(100);

    send_response_and_delete(response);
}

void handle_initialized(JSON *notification) {
    /* Just ignore it for now... */
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

    JSON *uri = add_json_array_as(changes, get_lsp_uri_string_from_request(request));
    JSON *change = add_json_object_to_array(uri);

    JSON *params = get_json_item(request, "params");

    int start_line, start_character, end_line, end_character;
    get_lsp_range_positions(params, &start_line, &start_character, &end_line, &end_character);

    add_lsp_range(change, start_line, start_character, end_line, end_character);

    add_lsp_new_text(change, "Dummy Text");

    send_response_and_delete(response);
}

void handle_execute_command(JSON *request) {
    log_trace("LSP: Handling 'workspace/executeCommand'");

    JSON *params = get_json_item(request, "params");
    const JSON *command = get_json_item(params, "command");
    const char *cmd = command->valuestring;

    if (command && strcmp(cmd, "dummyCommand") == 0) {
        log_info("LSP: Executing dummy command");
        // Perform some action or return a result
    }

    JSON *response = cJSON_CreateNull();
    send_response_and_delete(response);
}

void handle_did_open(JSON *notification) {
    log_trace("LSP: Handling 'textDocument/didOpen'");

    JSON *params = get_json_item(notification, "params");

    JSON *textDocument = get_json_item(params, "textDocument");
    char *uri = get_json_string_item(textDocument, "uri");
    const char *languageId = get_json_string_item(textDocument, "languageId");
    const char *text = get_json_string_item(textDocument, "text");

    log_trace("LSP: Opened file '%s', language '%s', text = '%s'", uri, languageId, text);

    addFileNameToFileTable(filename_from_uri(uri));

    // TODO Ensure file is in the fileTable and put the text into an EditorBuffer

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
    log_trace("LSP: Handling 'method not found'");

    JSON *response = create_lsp_message_with_id(id_of_request(request));

    JSON *error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", -32601);
    cJSON_AddStringToObject(error, "message", "Method not found");
    cJSON_AddItemToObject(response, "error", error);

    send_response_and_delete(response);
}
