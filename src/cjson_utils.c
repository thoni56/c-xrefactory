#include "cjson_utils.h"

#include <string.h>
#include <cjson/cJSON.h>


/* ============================================================== */

double id_of_request(cJSON *request) {
    return cJSON_GetObjectItem(request, "id")->valuedouble;
}

const char *get_uri_string_from_request(cJSON *request) {
    cJSON *response_uri_item = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetObjectItem(request, "params"), "textDocument"), "uri");
    return response_uri_item->valuestring;
}


/* ============================================================== */

cJSON *create_lsp_message_with_id(double id) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);
    return response;
}

cJSON *add_array_as(cJSON *target, const char *name) {
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToObject(target, name, array);
    return array;
}

/* ============================================================== */

bool cjson_equals(const cJSON *a, const cJSON *b) {
    // Check for nulls or type mismatch
    if (a == NULL || b == NULL || a->type != b->type) {
        return false;
    }

    switch (a->type) {
        case cJSON_False:
        case cJSON_True:
            return true;  // Booleans are equal if they have the same type

        case cJSON_NULL:
            return true;  // NULLs are always equal

        case cJSON_Number:
            return a->valuedouble == b->valuedouble;

        case cJSON_String:
            return strcmp(a->valuestring, b->valuestring) == 0;

        case cJSON_Array: {
            const cJSON *a_item = a->child;
            const cJSON *b_item = b->child;
            while (a_item && b_item) {
                if (!cjson_equals(a_item, b_item)) {
                    return false;
                }
                a_item = a_item->next;
                b_item = b_item->next;
            }
            return a_item == NULL && b_item == NULL;  // Ensure arrays have the same length
        }

        case cJSON_Object: {
            const cJSON *a_item = NULL;
            cJSON_ArrayForEach(a_item, a) {
                const cJSON *b_item = cJSON_GetObjectItemCaseSensitive(b, a_item->string);
                if (!b_item || !cjson_equals(a_item, b_item)) {
                    return false;
                }
            }
            return true;
        }

        default:
            return false;  // Unsupported type
    }
}
