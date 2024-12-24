#include "lsp_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

#include "lsp_errors.h"
#include "lsp_handler.h"
#include "log.h"


LspReturnCode dispatch_lsp_request(cJSON *request) {

    const cJSON *method = cJSON_GetObjectItem(request, "method");
    if (method && cJSON_IsString(method)) {
        if (strcmp(method->valuestring, "initialize") == 0) {
            log_trace("LSP: Dispatched 'initialize'");
            handle_initialize(request);
            return LSP_RETURN_OK;
        } else if (strcmp(method->valuestring, "exit") == 0) {
            log_trace("LSP: Dispatched 'initialize'");
            handle_exit(request);
            return LSP_RETURN_EXIT;
        }
    }
    return LSP_RETURN_ERROR_UNSUPPORTED_METHOD;  // Method not found or unsupported
}
