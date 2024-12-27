#ifndef CJSON_UTILS_H_INCLUDED
#define CJSON_UTILS_H_INCLUDED

#include <stdbool.h>
#include <cjson/cJSON.h>


extern double id_of_request(cJSON *request);
extern const char *get_uri_string_from_request(cJSON *request);

extern cJSON *create_lsp_message_with_id(double id);
extern cJSON *add_lsp_action(cJSON *target, const char *name, const char *kind);
extern cJSON *add_lsp_range(cJSON *target, int start_line, int start_character, int end_line, int end_character);
extern cJSON *add_lsp_new_text(cJSON *target, const char *new_text);

extern cJSON *add_json_array_as(cJSON *target, const char *name);
extern cJSON *add_json_item(cJSON *target, const char *name);
extern cJSON *add_json_object_to_array(cJSON *target);
extern cJSON *add_json_bool(cJSON *target, const char *name, bool value);
extern cJSON *add_json_string(cJSON *target, const char *name, const char *value);

extern bool json_equals(const cJSON *a, const cJSON *b);


#endif
