#ifndef LSP_HANDLER_H_INCLUDED
#define LSP_HANDLER_H_INCLUDED

#include "json_utils.h"


/* Requests will *not* be deleted since they are allocated by caller */
extern void handle_initialize(JSON *request);
extern void handle_code_action(JSON *request);
extern void handle_did_open(JSON *notification);
extern void handle_shutdown(JSON *request);
extern void handle_exit(JSON *request);
extern void handle_method_not_found(JSON *request);

#endif
