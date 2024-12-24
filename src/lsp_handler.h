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
 * @brief Handle a 'shutdown' request
 *
 * @param[in] request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_shutdown(cJSON *request);

/**
 * @brief Handle an 'exit' request
 *
 * @param[in] request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_exit(cJSON *request);

/**
 * @brief Handle an unknown or unimplemented request
 *
 * @param[in] request - the request in JSON format, will *not* be deleted
 *
 */
extern void handle_method_not_found(cJSON *request);

#endif
