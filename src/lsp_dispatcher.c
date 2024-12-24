#include "lsp_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

#include "lsp_errors.h"
#include "lsp_handler.h"
#include "log.h"


LspReturnCode dispatch_lsp_message(cJSON *message) {

    const cJSON *method = cJSON_GetObjectItem(message, "method");
    if (method && cJSON_IsString(method)) {
        if (strcmp(method->valuestring, "initialize") == 0) {
            log_trace("LSP: Dispatched 'initialize'");
            handle_initialize(message);
            return LSP_RETURN_OK;
        } else if (strcmp(method->valuestring, "shutdown") == 0) {
            log_trace("LSP: Dispatched 'shutdown'");
            handle_shutdown(message);
            return LSP_RETURN_OK;
       } else if (strcmp(method->valuestring, "exit") == 0) {
            log_trace("LSP: Dispatched 'initialize'");
            handle_exit(message);
            return LSP_RETURN_EXIT;
        } else {
            if (cJSON_GetObjectItem(message, "id") != NULL) {
                log_trace("LSP: Received unsupported request '%s'", method->valuestring);
                handle_method_not_found(message);
            } else
                log_trace("LSP: Received unsupported notification '%s'", method->valuestring);
            return LSP_RETURN_ERROR_METHOD_NOT_FOUND;
        }

    }
    return LSP_RETURN_ERROR_JSON_PARSE_ERROR;
}
