#include "lsp_adapter.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>

#include "filetable.h"
#include "json_utils.h"
#include "lsp_utils.h"
#include "options.h"
#include "server.h"
#include "session.h"


extern void totalTaskEntryInitialisations(void);

JSON *findDefinition(const char *uri, JSON *position) {
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
    
    /* Set up minimal server options for definition lookup */
    Options savedOptions = options;  /* Save current state */
    
    /* Configure for definition lookup */
    options.mode = ServerMode;
    options.xref2 = true;
    options.serverOperation = OLO_PUSH;
    options.olCursorOffset = cursorOffset;
    
    /* Clear and set input file */
    options.inputFiles = NULL;
    addToStringListOption(&options.inputFiles, filePath);
    
    /* Initialize and process */
    totalTaskEntryInitialisations();
    processFileArguments();
    
    /* Run the server pipeline to find definition */
    int argc = 0;
    char **argv = NULL;
    int nargc = 0;
    char **nargv = NULL;
    bool firstPass;
    
    callServer(argc, argv, nargc, nargv, &firstPass);
    
    /* Extract results from browser stack */
    JSON *result = NULL;
    if (sessionData.browserStack.top && sessionData.browserStack.top->current) {
        Position defPosition = sessionData.browserStack.top->current->position;
        char *defFileName = getFileItemWithFileNumber(defPosition.file)->name;
        
        /* Convert c-xrefactory position to LSP coordinates */
        /* c-xrefactory uses 1-based line numbers, 0-based column numbers */
        /* LSP uses 0-based line and character numbers */
        int defLine = defPosition.line - 1;  /* Convert to 0-based */
        int defCharacter = defPosition.col;  /* Already 0-based column within line */
        
        /* Create LSP response */
        result = cJSON_CreateObject();
        char defUriBuffer[1024];
        snprintf(defUriBuffer, sizeof(defUriBuffer), "file://%s", defFileName);
        add_json_string(result, "uri", defUriBuffer);
        add_lsp_range(result, defLine, defCharacter, defLine, defCharacter + 1);
    }
    
    /* Restore options */
    options = savedOptions;
    
    return result;
}
