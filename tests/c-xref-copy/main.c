#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "argumentsvector.h"
#include "commandlogger.h"
#include "commons.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "lsp.h"
#include "memory.h"
#include "proto.h"
#include "refactory.h"
#include "server.h"
#include "stackmemory.h"
#include "startup.h"
#include "options.h"
#include "xref.h"
#include "yylex.h"


/* initLogging() is called as the first thing in main() so we look for log command line options here */
static void initLogging(ArgumentsVector args) {
    char fileName[MAX_FILE_NAME_SIZE+1] = "";
    LogLevel log_level = LOG_ERROR;
    LogLevel console_level = LOG_FATAL;

    for (int i=0; i<args.argc; i++) {
        /* Levels in the log file, if enabled */
        if (strncmp(args.argv[i], "-log=", 5)==0)
            strcpy(fileName, &args.argv[i][5]);
        if (strcmp(args.argv[i], "-debug") == 0)
            log_level = LOG_DEBUG;
        if (strcmp(args.argv[i], "-trace") == 0)
            log_level = LOG_TRACE;
        /* Levels on the console */
        if (strcmp(args.argv[i], "-errors") == 0)
            console_level = LOG_ERROR;
        if (strcmp(args.argv[i], "-warnings") == 0)
            console_level = LOG_WARN;
        if (strcmp(args.argv[i], "-infos") == 0)
            console_level = LOG_INFO;
    }

    /* Was there a filename, -log given? */
    if (fileName[0] != '\0') {
        FILE *logFile = openFile(fileName, "w");
        if (logFile != NULL)
            log_add_fp(logFile, log_level);
    }

    /* Always log errors and above to console */
    log_set_level(console_level);

    /* Should we log the arguments? */
    if (true)
        logCommands(args);
}

static void checkForStartupDelay(int argc, char *argv[]) {
    for (int i=0; i<argc; i++) {
        if (strncmp(argv[i], "-delay=", 7)==0) {
            sleep(atoi(&argv[i][7]));
            return;
        }
    }
}

/* *********************************************************************** */
/* **************************       MAIN      **************************** */
/* *********************************************************************** */

int main(int argc, char *argv[]) {
    checkForStartupDelay(argc, argv);

    ArgumentsVector args = {.argc = argc, .argv = argv};

    /* Options are read very late down below, so we need to setup logging before then */
    initLogging(args);
    ENTER();

    /* And if we want to run the experimental LSP server, ignore anything else */
    if (want_lsp_server(args))
        return lsp_server(stdin);

    /* else continue with legacy implementation */
    if (setjmp(memoryResizeJumpTarget) != 0) {
        /* CX_ALLOCC always makes one longjmp back to here before we can
           start processing for real ... Allocating initial CX memory */
        if (cxResizingBlocked) {
            FATAL_ERROR(ERR_ST, "cx_memory resizing required, see file TROUBLES",
                       EXIT_FAILURE);
        }
    }

    currentPass = ANY_PASS;
    totalTaskEntryInitialisations();
    mainTaskEntryInitialisations(args);

    if (options.mode == RefactoryMode)
        refactory();
    if (options.mode == XrefMode)
        xref(args);
    if (options.mode == ServerMode)
        server(args);

    if (options.statistics) {
        printMemoryStatistics();
        printOptionsMemoryStatistics();
        yylexMemoryStatistics();
        fileTableMemoryStatistics();
        stackMemoryStatistics();
    }

    LEAVE();
    return 0;
}
