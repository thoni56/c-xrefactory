#include "lsp_adapter.h"

#include <cjson/cJSON.h>

#include "commons.h"
#include "cxref.h"
#include "json_utils.h"
#include "main.h"
#include "options.h"
#include "server.h"


extern void totalTaskEntryInitialisations(void);

JSON *findDefinition(const char *uri, JSON *position) {
    /* Some random inits that should not be here actually */
    options.javaFilesSuffixes = "java";
    options.cFilesSuffixes = "c";

    totalTaskEntryInitialisations();

    int argc = 0;
    char **argv = NULL;
    mainTaskEntryInitialisations(argc, argv);

    /* Replicating server() call... */
    openOutputFile("c-xrefactory-lsp-output");

    int nargc = 0;
    char **nargv = NULL;
    // initServer(nargc, nargv);
    clearAvailableRefactorings();

    /* Options parsing */
    options.mode = ServerMode;
    options.xref2 = true;
    options.serverOperation = OLO_PUSH;
    options.olCursorOffset = 55; /* Convert from position */
    addToStringListOption(&options.inputFiles, (char*)&uri[7]);

    processFileArguments();

    bool firstPass;
    callServer(argc, argv, nargc, nargv, &firstPass);

    answerEditAction();

    /* Dummy implementation for now - use same uri and fake a position */
    JSON *definition = cJSON_CreateObject();
    add_json_string(definition, "uri", uri);
    add_lsp_range(definition, 2, 12, 2, 15);
    return definition;
}
