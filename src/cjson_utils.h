#ifndef CJSON_UTILS_H_INCLUDED
#define CJSON_UTILS_H_INCLUDED

#include <stdbool.h>
#include <cjson/cJSON.h>


extern double id_of_request(cJSON *request);
extern const char *get_uri_string_from_request(cJSON *request);

extern cJSON *create_lsp_message_with_id(double id);
extern cJSON *add_array_as(cJSON *target, const char *name);

extern bool cjson_equals(const cJSON *a, const cJSON *b);


#endif
