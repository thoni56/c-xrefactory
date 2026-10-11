#include "lsp_dispatcher.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "json_utils.h"
#include "lsp_errors.h"
#include "lsp_handler.h"
#include "log.h"

typedef void (*Handler)(JSON *message);

typedef struct dispatchEntry {
    const char *method;
    Handler handler;
} DispatchEntry;

DispatchEntry dispatch[] = {
    {"initialize", handle_initialize},
    {"initialized", handle_initialized},
    {"shutdown", handle_shutdown},
    {"exit", handle_exit},

    {"textDocument/didOpen", handle_did_open},
    {"textDocument/codeAction", handle_code_action},
    {"textDocument/definition", handle_definition_request},
    {"workspace/executeCommand", handle_execute_command},

    {"$/cancelRequest", handle_cancel}
};

#define ARRAY_SIZE(table) sizeof(table) / sizeof(table[0])

static bool shutdownRequested = false;

LspReturnCode dispatch_lsp_message(JSON *message) {

    const char *method = get_json_string_item(message, "method");
    if (shutdownRequested && strcmp(method, "exit") != 0) {
        if (get_json_item(message, "id") != NULL)
            handle_invalid_request(message);
        return LSP_RETURN_OK;
    }
    for (int i=0; i < ARRAY_SIZE(dispatch); i++) {
        if (strcmp(method, dispatch[i].method) == 0) {
            log_trace("LSP: Dispatched '%s'", method);
            dispatch[i].handler(message);
            if (strcmp(method, "shutdown") == 0)
                shutdownRequested = true;
            if (strcmp(method, "exit") == 0)
                return shutdownRequested ? LSP_RETURN_EXIT : LSP_RETURN_EXIT_WITHOUT_SHUTDOWN;
            else
                return LSP_RETURN_OK;
        }
    }
    if (get_json_item(message, "id") != NULL) {
        log_trace("LSP: Received unsupported request '%s'", method);
        handle_method_not_found(message);
    } else
        log_trace("LSP: Received unsupported notification '%s'", method);
    return LSP_RETURN_ERROR_METHOD_NOT_FOUND;
}
