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



static int needToProcessInputFile(void) {
    return options.serverOperation==OLO_COMPLETION
           || options.serverOperation==OLO_SEARCH
           || options.serverOperation==OLO_EXTRACT
           || options.serverOperation==OLO_TAG_SEARCH
           || options.serverOperation==OLO_SET_MOVE_TARGET
           || options.serverOperation==OLO_SET_MOVE_CLASS_TARGET
           || options.serverOperation==OLO_SET_MOVE_METHOD_TARGET
           || options.serverOperation==OLO_GET_CURRENT_CLASS
           || options.serverOperation==OLO_GET_CURRENT_SUPER_CLASS
           || options.serverOperation==OLO_GET_METHOD_COORD
           || options.serverOperation==OLO_GET_CLASS_COORD
           || options.serverOperation==OLO_GET_ENV_VALUE
           || creatingOlcxRefs()
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
    if (creatingOlcxRefs())
        olcxPushEmptyStackItem(&sessionData.browserStack);
    if (needToProcessInputFile()) {
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
