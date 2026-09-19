#ifndef LSP_HANDLER_H_INCLUDED
#define LSP_HANDLER_H_INCLUDED

#include "json_utils.h"

#include "position.h"
#include "reference_database.h"

typedef struct lspPosition {
    int line;
    int character;
} LspPosition;

typedef struct lspLocation {
    char *uri;
    struct {
        Position start;
        Position end;
    } range;
} LspLocation;

/* Requests will *not* be deleted since they are allocated by caller */
extern void handle_initialize(JSON *request);
extern void handle_initialized(JSON *notification);
extern void handle_did_open(JSON *notification);
extern void handle_definition_request(JSON *request);
extern void handle_code_action(JSON *request);
extern void handle_execute_command(JSON *request);
extern void handle_cancel(JSON *notification);
extern void handle_shutdown(JSON *request);
extern void handle_exit(JSON *request);
extern void handle_method_not_found(JSON *request);

/* Reference database accessors for testing */
extern ReferenceDatabase *getReferenceDatabase(void);
extern void setReferenceDatabase(ReferenceDatabase *db);

#endif
