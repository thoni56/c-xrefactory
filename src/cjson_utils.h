#ifndef CJSON_UTILS_H_INCLUDED
#define CJSON_UTILS_H_INCLUDED

#include <stdbool.h>
#include <cjson/cJSON.h>


extern cJSON *create_response(double id);

extern double id_of_request(cJSON *request);

extern bool cjson_equals(const cJSON *a, const cJSON *b);

#endif
