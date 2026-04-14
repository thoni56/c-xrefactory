#include "lsp_adapter.h"

#include <stdio.h>
#include <string.h>

#include "head.h"
#include "filetable.h"
#include "json_utils.h"
#include "log.h"
#include "lsp_utils.h"
#include "position.h"
#include "referenceableitem.h"
#include "referenceableitemtable.h"
#include "usage.h"


/* Context for searching the ReferenceableItemTable by position */
typedef struct {
    Position targetPosition;
    ReferenceableItem *found;
} FindReferenceableContext;

/* Get the bare (source-level) name from a possibly qualified linkName */
static const char *bareNameOf(const char *linkName) {
    const char *bare = linkName;
    for (const char *p = linkName; *p != '\0'; p++) {
        if (*p == LINK_NAME_SEPARATOR || *p == LINK_NAME_COLLATE_SYMBOL)
            bare = p + 1;
    }
    return bare;
}

/* Check if target position is within the reference identifier */
static bool positionIsWithinReference(Position refStart, Position target, const char *linkName) {
    if (refStart.file != target.file || refStart.line != target.line)
        return false;

    int length = strlen(bareNameOf(linkName));
    return target.col >= refStart.col && target.col < refStart.col + length;
}

/* Callback for mapOverReferenceableItemTableWithPointer */
static void checkReferenceableItemAtPosition(ReferenceableItem *item, void *contextPtr) {
    FindReferenceableContext *context = (FindReferenceableContext *)contextPtr;

    if (context->found != NULL)
        return;

    for (Reference *ref = item->references; ref != NULL; ref = ref->next) {
        if (positionIsWithinReference(ref->position, context->targetPosition, item->linkName)) {
            log_trace("Found referenceable '%s' at %d:%d (target was %d:%d, length %zu)",
                     item->linkName, ref->position.line, ref->position.col,
                     context->targetPosition.line, context->targetPosition.col,
                     strlen(item->linkName));
            context->found = item;
            return;
        }
    }
}

static ReferenceableItem *findReferenceableItemWithReferenceAt(Position position) {
    FindReferenceableContext context = {
        .targetPosition = position,
        .found = NULL
    };

    mapOverReferenceableItemTableWithPointer(checkReferenceableItemAtPosition, &context);

    return context.found;
}

static Position extractDefinitionPosition(ReferenceableItem *item) {
    for (Reference *ref = item->references; ref != NULL; ref = ref->next) {
        if (isDefinitionUsage(ref->usage))
            return ref->position;
    }
    return NO_POSITION;
}


JSON *findDefinition(const char *uri, JSON *positionJson) {
    ENTER();

    /* Extract LSP position parameters */
    JSON *lineJson = cJSON_GetObjectItem(positionJson, "line");
    JSON *characterJson = cJSON_GetObjectItem(positionJson, "character");

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
    Position position = {
        .file = fileNumber,
        .line = lspLine + 1,     // Convert 0-based to 1-based
        .col = lspCharacter      // Both use 0-based columns
    };

    /* Find the referenceable at cursor position */
    ReferenceableItem *item = findReferenceableItemWithReferenceAt(position);
    if (item == NULL) {
        log_trace("findDefinition: No referenceable found at position %d:%d:%d", position.file, position.line, position.col);
        LEAVE();
        return NULL;
    }

    /* Find the definition */
    Position definition = extractDefinitionPosition(item);
    if (definition.file == NO_FILE_NUMBER) {
        log_trace("findDefinition: Definition has invalid file number");
        LEAVE();
        return NULL;
    }

    FileItem *fileItem = getFileItemWithFileNumber(definition.file);
    if (fileItem == NULL) {
        log_trace("findDefinition: Could not get file item for file number %d", definition.file);
        LEAVE();
        return NULL;
    }

    char *definitionFilePath = fileItem->name;

    /* Create URI from file path */
    char uri_buffer[1024];
    snprintf(uri_buffer, sizeof(uri_buffer), "file://%s", definitionFilePath);

    /* Create location JSON object */
    cJSON *location = cJSON_CreateObject();
    cJSON_AddStringToObject(location, "uri", uri_buffer);

    /* Add range spanning the entire identifier */
    /* Convert c-xrefactory line (1-based) to LSP line (0-based) */
    int lspDefLine = definition.line - 1;
    int lspDefStartCol = definition.col;
    int identifierLength = strlen(bareNameOf(item->linkName));
    int lspDefEndCol = lspDefStartCol + identifierLength;

    add_lsp_range(location, lspDefLine, lspDefStartCol, lspDefLine, lspDefEndCol);

    log_trace("findDefinition: Returning location at %s:%d:%d-%d", definitionFilePath, lspDefLine, lspDefStartCol, lspDefEndCol);

    LEAVE();
    return location;
}
