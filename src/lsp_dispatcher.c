#include "lsp_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

#include "lsp_handler.h"
#include "log.h"


int dispatch_lsp_request(cJSON *request) {

    const cJSON *method = cJSON_GetObjectItem(request, "method");
    if (method && cJSON_IsString(method) && strcmp(method->valuestring, "initialize") == 0) {
        log_trace("LSP: Dispatched 'initialize'");
        handle_initialize(request);

        cJSON_Delete(request);
        return 0;  // Success
    }

    cJSON_Delete(request);
    return -1;  // Method not found or unsupported
}
