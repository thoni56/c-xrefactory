#include "lsp_dispatcher.h"

#include <string.h>
#include <stdio.h>

#include "json_utils.h"
#include "lsp_errors.h"
#include "lsp_handler.h"
#include "log.h"


LspReturnCode dispatch_lsp_message(JSON *message) {

    const char *method = get_json_string_item(message, "method");
    if (strcmp(method, "initialize") == 0) {
        log_trace("LSP: Dispatched 'initialize'");
        handle_initialize(message);
        return LSP_RETURN_OK;
    } else if (strcmp(method, "shutdown") == 0) {
        log_trace("LSP: Dispatched 'shutdown'");
        handle_shutdown(message);
        return LSP_RETURN_OK;
    } else if (strcmp(method, "textDocument/codeAction") == 0) {
        log_trace("LSP: Dispatched 'textDocument/codeAction'");
        handle_code_action(message);
        return LSP_RETURN_OK;
    } else if (strcmp(method, "exit") == 0) {
        log_trace("LSP: Dispatched 'initialize'");
        handle_exit(message);
        return LSP_RETURN_EXIT;
    } else {
        if (get_json_item(message, "id") != NULL) {
            log_trace("LSP: Received unsupported request '%s'", method);
            handle_method_not_found(message);
        } else
            log_trace("LSP: Received unsupported notification '%s'", method);
        return LSP_RETURN_ERROR_METHOD_NOT_FOUND;
    }

    return LSP_RETURN_ERROR_JSON_PARSE_ERROR;
}
