#ifndef LSP_HANDLER_H
#define LSP_HANDLER_H

#include <cjson/cJSON.h>

/**
 * @brief Handle an 'initialize' request
 *
 * @param[in] request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_initialize(cJSON *request);

/**
 * @brief Handle an 'exit' request
 *
 * @param[in] request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_exit(cJSON *request);

#endif
