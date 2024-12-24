#ifndef LSP_DISPATCHER_H
#define LSP_DISPATCHER_H

#include <cjson/cJSON.h>


/**
 * @brief Dispatches the incoming request to the correct handler
 *
 * @param request - The incoming request in JSON format, *not* freed by dispatcher
 */
extern int dispatch_lsp_request(cJSON *request);

#endif
