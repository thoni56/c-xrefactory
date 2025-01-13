#include "lsp_dispatcher.h"

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
    {"workspace/executeCommand", handle_execute_command},

    {"$/cancelRequest", handle_cancel}
};

LspReturnCode dispatch_lsp_message(JSON *message) {

    const char *method = get_json_string_item(message, "method");
    for (int i=0; i < sizeof(dispatch)/sizeof(dispatch[0]); i++) {
        if (strcmp(method, dispatch[i].method) == 0) {
            log_trace("LSP: Dispatched '%s'", method);
            dispatch[i].handler(message);
            if (strcmp(method, "exit") == 0)
                return LSP_RETURN_EXIT;
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
