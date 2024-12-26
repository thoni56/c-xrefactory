#ifndef LSP_HANDLER_H
#define LSP_HANDLER_H

#include <cjson/cJSON.h>

/* Requests will *not* be deleted since they are allocated by caller */
extern void handle_initialize(cJSON *request);
extern void handle_code_action(cJSON *request);
extern void handle_shutdown(cJSON *request);
extern void handle_exit(cJSON *request);
extern void handle_method_not_found(cJSON *request);

#endif
