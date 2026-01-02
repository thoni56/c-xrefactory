#include "lsp_adapter.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>

#include "filetable.h"
#include "json_utils.h"
#include "log.h"
#include "lsp_utils.h"
#include "position.h"
#include "reference_database.h"


JSON *findDefinition(const char *uri, JSON *position) {
    log_trace("findDefinition: ENTRY");
    /* Extract LSP position parameters */
    JSON *lineJson = cJSON_GetObjectItem(position, "line");
    JSON *characterJson = cJSON_GetObjectItem(position, "character");

    if (!lineJson || !characterJson) {
        return NULL;
    }

    int lspLine = lineJson->valueint;
    int lspCharacter = characterJson->valueint;

    /* Convert URI to file path */
    char *filePath = uriToFilePath(uri);

    /* TODO: Convert file path to file number
     * For now we don't have the file number, so we can't create a proper Position.
     * This will need to be implemented when we integrate with file parsing.
     */
    (void)filePath;

    /* Create Position from LSP coordinates (LSP is 0-based, c-xrefactory is 1-based for lines) */
    Position pos = {
        .file = NO_FILE_NUMBER,  // TODO: Get actual file number
        .line = lspLine + 1,     // Convert 0-based to 1-based
        .col = lspCharacter      // Both use 0-based columns
    };

    /* Query the reference database */
    ReferenceDatabase *db = createReferenceDatabase();
    ReferenceableResult result = findReferenceableAt(db, filePath, pos);
    destroyReferenceDatabase(db);

    /* If not found, return NULL */
    if (!result.found) {
        log_trace("findDefinition: No referenceable found at position");
        return NULL;
    }

    /* Convert definition position to LSP format */
    /* TODO: Get file name from definition.file number */
    /* For now, we can't complete the implementation without file number mapping */
    log_trace("findDefinition: Found referenceable but can't return location without file number mapping");

    (void)result;  // Suppress unused warning
    return NULL;
}
