#include "lsp_handler.h"

#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "editorbuffer.h"
#include "editorbuffertable.h"
#include "filetable.h"
#include "json_utils.h"
#include "log.h"
#include "lsp_adapter.h"
#include "lsp_sender.h"
#include "parsing.h"
#include "reference_database.h"


/* Global reference database for the LSP session */
static ReferenceDatabase *referenceDatabase = NULL;

ReferenceDatabase *getReferenceDatabase(void) {
    return referenceDatabase;
}

void setReferenceDatabase(ReferenceDatabase *db) {
    referenceDatabase = db;
}

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
    //add_json_bool(capabilities, "codeActionProvider", true);
    add_json_bool(capabilities, "definitionProvider", true);
    add_json_string(response, "positionEncoding", "utf-8");

    initFileTable(100);
    initEditorBufferTable();

    /* Initialize parsing subsystem */
    initializeParsingSubsystem();

    /* Create the reference database for this LSP session */
    referenceDatabase = createReferenceDatabase();
    log_trace("LSP: Reference database created");

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
    const char *uri = get_json_string_item(textDocument, "uri");
    const char *text = get_json_string_item(textDocument, "text");

    log_trace("LSP: Opened file '%s', text = '%s'", uri, text);

    char *fileName = filename_from_uri(uri);
    EditorBuffer *buffer = createNewEditorBuffer(fileName, NULL, time(NULL), strlen(text));
    loadTextIntoEditorBuffer(buffer, time(NULL), text);
    buffer->textLoaded = true;  /* Mark text as loaded for getOpenedAndLoadedEditorBuffer() */

    /* Parse the file to populate the reference database.
     * Language is derived from filename extension by parseToCreateReferences(). */
    log_trace("LSP: Parsing file '%s'", buffer->fileName);
    parseToCreateReferences(buffer->fileName);
    log_trace("LSP: Finished parsing file '%s'", buffer->fileName);
}


void handle_definition_request(JSON *request) {
    log_trace("LSP: Handling 'textDocument/definition'");

    JSON *params = get_json_item(request, "params");
    JSON *textDocument = get_json_item(params, "textDocument");
    char *uri = get_json_string_item(textDocument, "uri");
    JSON *position = get_json_item(params, "position");

    (void)uri;
    (void)position;

    JSON *definition = findDefinition(uri, position);

    JSON *response = create_lsp_response(id_of_request(request), definition);

    send_response_and_delete(response);
}

void handle_cancel(JSON *notification) {
    // For now there is nothing that can be cancelled
}

void handle_shutdown(JSON *request) {
    log_trace("LSP: Handling 'shutdown'");

    /* Destroy the reference database */
    if (referenceDatabase != NULL) {
        destroyReferenceDatabase(referenceDatabase);
        referenceDatabase = NULL;
        log_trace("LSP: Reference database destroyed");
    }

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
