#include "lsp_adapter.h"

#include "json_utils.h"
#include <cjson/cJSON.h>


JSON *findDefinition(const char *uri, JSON *position) {
    /* Dummy implementation for now - use same uri and fake a position */
    JSON *definition = cJSON_CreateObject();
    add_json_string(definition, "uri", uri);
    add_lsp_range(definition, 2, 12, 2, 15);
    return definition;
}
