#include "lsp_adapter.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>

#include "json_utils.h"
#include "log.h"
#include "lsp_utils.h"


JSON *findDefinition(const char *uri, JSON *position) {
    log_trace("findDefinition: ENTRY");
    /* Extract LSP position parameters */
    JSON *lineJson = cJSON_GetObjectItem(position, "line");
    JSON *characterJson = cJSON_GetObjectItem(position, "character");

    if (!lineJson || !characterJson) {
        return NULL;
    }

    int line = lineJson->valueint;
    int character = characterJson->valueint;

    /* Convert URI to file path and LSP position to byte offset */
    char *filePath = uriToFilePath(uri);
    int cursorOffset = lspPositionToByteOffset(filePath, line, character);

    /* TODO: Implement goto-definition without legacy server infrastructure
     *
     * What we need:
     * 1. Parse the file (or use already-parsed EditorBuffer from didOpen)
     * 2. Find the symbol at cursorOffset
     * 3. Look up its definition in the symbol database
     * 4. Return the definition location
     *
     * For now, return NULL to indicate "not found"
     */
    (void)filePath;
    (void)cursorOffset;

    return NULL;
}
