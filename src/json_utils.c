#include "json_utils.h"

#include <string.h>
#include <cjson/cJSON.h>


/* ============================================================== */

double id_of_request(JSON *request) {
    return cJSON_GetObjectItem(request, "id")->valuedouble;
}

const char *get_uri_string_from_request(JSON *request) {
    cJSON *response_uri_item = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetObjectItem(request, "params"), "textDocument"), "uri");
    return response_uri_item->valuestring;
}


/* ============================================================== */

JSON *create_lsp_message_with_id(double id) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);
    return response;
}

JSON *add_json_array_as(JSON *target, const char *name) {
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToObject(target, name, array);
    return array;
}

JSON *add_json_object_to_array(JSON *target) {
    cJSON *object = cJSON_CreateObject();
    cJSON_AddItemToArray(target, object);
    return object;
}

JSON *add_lsp_action(JSON *target, const char *title, const char *kind) {
    cJSON *action = cJSON_CreateObject();
    cJSON_AddItemToArray(target, action);
    cJSON_AddStringToObject(action, "title", title);
    cJSON_AddStringToObject(action, "kind", kind);
    return action;
}

JSON *add_json_item(JSON *target, const char *name) {
    cJSON *item = cJSON_CreateObject();
    cJSON_AddItemToObject(target, name, item);
    return item;
}

JSON *add_lsp_range(JSON *target, int start_line, int start_character,
                 int end_line, int end_character) {
    cJSON *range = cJSON_CreateObject();
    cJSON *start = cJSON_CreateObject();
    cJSON_AddNumberToObject(start, "line", start_line);
    cJSON_AddNumberToObject(start, "character", start_character);
    cJSON_AddItemToObject(range, "start", start);
    cJSON *end = cJSON_CreateObject();
    cJSON_AddNumberToObject(end, "line", end_line);
    cJSON_AddNumberToObject(end, "character", end_character);
    cJSON_AddItemToObject(range, "end", end);
    cJSON_AddItemToObject(target, "range", range);
    return range;
}

JSON *add_lsp_new_text(JSON *target, const char *new_text) {
    return cJSON_AddStringToObject(target, "newText", new_text);
}

JSON *add_json_string(JSON *target, const char *name, const char *value) {
    return cJSON_AddStringToObject(target, name, value);
}

JSON *add_json_bool(JSON *target, const char *name, bool value) {
    return cJSON_AddBoolToObject(target, name, value);
}


/* ============================================================== */

bool json_equals(const JSON *a, const JSON *b) {
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
                if (!json_equals(a_item, b_item)) {
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
                if (!b_item || !json_equals(a_item, b_item)) {
                    return false;
                }
            }
            return true;
        }

        default:
            return false;  // Unsupported type
    }
}

JSON *parse_json(const char *string) {
    return cJSON_Parse(string);
}

char *print_json(JSON *tree) {
    return cJSON_PrintUnformatted(tree);
}
