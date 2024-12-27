#ifndef LSP_DISPATCHER_H
#define LSP_DISPATCHER_H

#include "lsp_errors.h"
#include "json_utils.h"


/**
 * @brief Dispatches the incoming request to the correct handler
 *
 * @param[in] request - The incoming request in JSON format, will *not* be deleted
 */
extern LspReturnCode dispatch_lsp_message(JSON *request);

#endif
