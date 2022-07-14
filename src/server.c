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


static char *presetEditServerFileDependingStatics(void) {
    fileProcessingStartTime = time(NULL);

    s_primaryStartPosition = noPosition;
    s_staticPrefixStartPosition = noPosition;

    // This is pretty stupid, there is always only one input file
    // in edit server, otherwise it is an error
    int fileIndex = 0;
    inputFilename = getNextScheduledFile(&fileIndex);
    if (fileIndex == -1) { /* No more input files... */
        // conservative message, probably macro invoked on nonsaved file, TODO: WTF?
        olOriginalComFileNumber = noFileIndex;
        return NULL;
    }

    /* TODO: This seems strange, we only assert that the first file is scheduled to process.
       Then reset all other files, why? */
    assert(getFileItem(fileIndex)->isScheduled);
    for (int i=getNextExistingFileIndex(fileIndex+1); i != -1; i = getNextExistingFileIndex(i+1)) {
        getFileItem(i)->isScheduled = false;
    }

    olOriginalComFileNumber = fileIndex;

    char *fileName = inputFilename;
    currentLanguage = getLanguageFor(fileName);

    // O.K. just to be sure, there is no other input file
    return fileName;
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

void server(int argc, char **argv) {
    int nargc;  char **nargv;
    bool firstPass;

    ENTER();
    cxResizingBlocked = true;
    firstPass = true;
    copyOptionsFromTo(&options, &savedOptions);
    for(;;) {
        currentPass = ANY_PASS;
        copyOptionsFromTo(&savedOptions, &options);
        getPipedOptions(&nargc, &nargv);
        // O.K. -o option given on command line should catch also file not found
        // message
        mainOpenOutputFile(options.outputFileName);
        //&dumpOptions(nargc, nargv);
        log_trace("Server: Getting request");
        initServer(nargc, nargv);
        if (communicationChannel==stdout && options.outputFileName!=NULL) {
            mainOpenOutputFile(options.outputFileName);
        }
        mainCallEditServer(argc, argv, nargc, nargv, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            mainAnswerEditAction();
        }
        //& options.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        editorCloseAllBuffers();
        closeMainOutputFile();
        if (options.serverOperation == OLO_EXTRACT)
            cache.cpIndex = 2; // !!!! no cache
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
