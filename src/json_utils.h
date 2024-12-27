#ifndef CJSON_UTILS_H_INCLUDED
#define CJSON_UTILS_H_INCLUDED

#include <stdbool.h>
#include <cjson/cJSON.h>

typedef cJSON JSON;

extern double id_of_request(JSON *request);
extern const char *get_uri_string_from_request(JSON *request);

extern JSON *create_lsp_message_with_id(double id);
extern JSON *add_lsp_action(JSON *target, const char *name, const char *kind);
extern JSON *add_lsp_range(JSON *target, int start_line, int start_character, int end_line, int end_character);
extern JSON *add_lsp_new_text(JSON *target, const char *new_text);

extern JSON *add_json_array_as(JSON *target, const char *name);
extern JSON *add_json_item(JSON *target, const char *name);
extern JSON *add_json_object_to_array(JSON *target);
extern JSON *add_json_bool(JSON *target, const char *name, bool value);
extern JSON *add_json_string(JSON *target, const char *name, const char *value);

extern bool json_equals(const JSON *a, const JSON *b);
extern JSON *parse_json(const char *string);
extern char *print_json(JSON *tree);
extern void delete_json(JSON *tree);

#endif
