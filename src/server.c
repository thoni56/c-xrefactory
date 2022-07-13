#include "server.h"

#include "cxref.h"
#include "options.h"
#include "complete.h"
#include "globals.h"
#include "caching.h"

const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};

void initServer(int nargc, char **nargv) {
    clearAvailableRefactorings();
    options.classpath = "";
    processOptions(nargc, nargv, PROCESS_FILE_ARGUMENTS); /* no include or define options */
    processFileArguments();
    if (options.serverOperation == OLO_EXTRACT)
        cache.cpIndex = 2; // !!!! no cache, TODO why is 2 = no cache?
    initCompletions(&collectedCompletions, 0, noPosition);
}
