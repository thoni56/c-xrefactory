#include "lsp_adapter.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>

#include "head.h"
#include "filetable.h"
#include "json_utils.h"
#include "log.h"
#include "lsp_utils.h"
#include "lsp_handler.h"
#include "position.h"
#include "reference_database.h"


JSON *findDefinition(const char *uri, JSON *position) {
    ENTER();

    /* Extract LSP position parameters */
    JSON *lineJson = cJSON_GetObjectItem(position, "line");
    JSON *characterJson = cJSON_GetObjectItem(position, "character");

    if (!lineJson || !characterJson) {
        LEAVE();
        return NULL;
    }

    int lspLine = lineJson->valueint;
    int lspCharacter = characterJson->valueint;

    /* Convert URI to file path */
    char *filePath = uriToFilePath(uri);

    /* Convert file path to file number */
    int fileNumber = getFileNumberFromFileName(filePath);
    if (fileNumber == -1) {
        log_trace("findDefinition: File not found in file table: %s", filePath);
        LEAVE();
        return NULL;
    }

    /* Create Position from LSP coordinates (LSP is 0-based, c-xrefactory is 1-based for lines) */
    Position pos = {
        .file = fileNumber,
        .line = lspLine + 1,     // Convert 0-based to 1-based
        .col = lspCharacter      // Both use 0-based columns
    };

    /* Query the reference database */
    ReferenceDatabase *db = getReferenceDatabase();
    if (db == NULL) {
        log_trace("findDefinition: No reference database available");
        return NULL;
    }
    ReferenceableResult result = findReferenceableAt(db, filePath, pos);

    /* If not found, return NULL */
    if (!result.found) {
        log_trace("findDefinition: No referenceable found at position");
        LEAVE();
        return NULL;
    }

    /* Get filename from file number */
    FileItem *fileItem = getFileItemWithFileNumber(result.definition.file);
    char *definitionFilePath = fileItem->name;

    /* Create URI from file path */
    char uri_buffer[1024];
    snprintf(uri_buffer, sizeof(uri_buffer), "file://%s", definitionFilePath);

    /* Create location JSON object */
    cJSON *location = cJSON_CreateObject();
    cJSON_AddStringToObject(location, "uri", uri_buffer);

    /* Add range with start and end positions */
    /* Convert c-xrefactory line (1-based) to LSP line (0-based) */
    int lspDefLine = result.definition.line - 1;
    int lspDefCol = result.definition.col;

    add_lsp_range(location, lspDefLine, lspDefCol, lspDefLine, lspDefCol);

    log_trace("findDefinition: Returning location at %s:%d:%d", definitionFilePath, lspDefLine, lspDefCol);

    LEAVE();
    return location;
}
