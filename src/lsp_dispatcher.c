#include "lsp_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>

#include "lsp_handler.h"


int dispatch_lsp_message(const char *message) {
    cJSON *root = cJSON_Parse(message);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON payload\n");
        return -1;
    }

    const cJSON *method = cJSON_GetObjectItem(root, "method");
    if (method && cJSON_IsString(method) && strcmp(method->valuestring, "initialize") == 0) {
        handle_initialize(root);

        cJSON_Delete(root);
        return 0;  // Success
    }

    cJSON_Delete(root);
    return -1;  // Method not found or unsupported
}
