#include "server.h"

#include "cxref.h"
#include "options.h"
#include "complete.h"
#include "globals.h"
#include "caching.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "commons.h"
#include "filetable.h"
#include "main.h"

const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};



static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_SEARCH
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_TARGET
           || operation==OLO_SET_MOVE_CLASS_TARGET
           || operation==OLO_SET_MOVE_METHOD_TARGET
           || operation==OLO_GET_CURRENT_CLASS
           || operation==OLO_GET_CURRENT_SUPER_CLASS
           || operation==OLO_GET_METHOD_COORD
           || operation==OLO_GET_CLASS_COORD
           || operation==OLO_GET_ENV_VALUE
           || isCreatingRefs(operation)
        ;
}


void initServer(int nargc, char **nargv) {
    clearAvailableRefactorings();
    options.classpath = "";
    processOptions(nargc, nargv, PROCESS_FILE_ARGUMENTS); /* no include or define options */
    processFileArguments();
    if (options.serverOperation == OLO_EXTRACT)
        cache.cpIndex = 2; // !!!! no cache, TODO why is 2 = no cache?
    initCompletions(&collectedCompletions, 0, noPosition);
}

void mainCallEditServer(int argc, char **argv,
                        int nargc, char **nargv,
                        bool *firstPass
) {
    ENTER();

    editorLoadAllOpenedBufferFiles();

    if (isCreatingRefs(options.serverOperation))
        olcxPushEmptyStackItem(&sessionData.browserStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            editServerProcessFile(argc, argv, nargc, nargv, firstPass);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            getFileItem(olOriginalComFileNumber)->isScheduled = false;
            // added [26.12.2002] because of loading options without input file
            inputFilename = NULL;
        }
    }
    LEAVE();
}
