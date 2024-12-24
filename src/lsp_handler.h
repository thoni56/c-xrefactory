#ifndef LSP_HANDLER_H
#define LSP_HANDLER_H

#include <cjson/cJSON.h>

/**
 * @brief Handle an 'initialize' request
 *
 * @param request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_initialize(cJSON *request);

#endif
