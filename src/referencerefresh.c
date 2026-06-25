#include "referencerefresh.h"
#include "referencerefresh.h"

#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "misc.h"
#include "parsing.h"
#include "referenceableitemtable.h"
#include "startup.h"


/* Parse a file using the full initializeFileProcessing machinery (project
 * discovery, options, checkpoint) with multi-pass support.  Saves and
 * restores REQUEST-level option fields that initOptions() inside
 * initializeFileProcessing would otherwise wipe. */
static void parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs) {
    int savedCursorOffset = options.cursorOffset;
    bool savedNoErrors = options.noErrors;
    ServerOperation savedServerOperation = options.serverOperation;

    inputFileName = fileName;
    ArgumentsVector emptyArgs = {.argc = 0, .argv = NULL};
    maxPasses = 1;
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        if (initializeFileProcessing(baseArgs, emptyArgs)) {
            options.cursorOffset = NO_CURSOR_OFFSET;
            options.noErrors = true;
            parseToCreateReferences(inputFileName);
            closeCharacterBuffer(&currentFile.characterBuffer);
            currentFile.characterBuffer.file = stdin;
        }
        currentFile.characterBuffer.isAtEOF = false;
    }

    options.cursorOffset = savedCursorOffset;
    options.noErrors = savedNoErrors;
    options.serverOperation = savedServerOperation;
}

void reparseStaleFile(int fileNumber, ArgumentsVector baseArgs) {
    removeReferenceableItemsForFile(fileNumber);
    parseFileWithFullInit(getFileItemWithFileNumber(fileNumber)->name, baseArgs);
}

/* Mark every compilation unit unparsed so the next entry refresh reparses it.
 * Called after a project-config change, which can invalidate any file's parse
 * (defines flip #ifdefs, -I re-resolves includes) with no cheap way to know
 * which — so treat it as a logical cold restart. Old refs are left in place
 * until reparseStaleFile clears and rebuilds each file as it is touched. */
void markAllCompilationUnitsStale(void) {
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fileItem->name))
            fileItem->lastParsedMtime = NULL_TIMESTAMP;
    }
}

/* Refresh references for a file. For a compilation unit, parse the
 * file directly. For a header, just strip its refs — the includer
 * CU's reparse (in Pass 1 or Pass 2) will re-emit them in proper CU
 * context. Standalone header parse would lose preprocessor context
 * and produce wrong refs. */
void reparseFile(int fileNumber, ArgumentsVector baseArgs) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    if (isCompilationUnit(fileItem->name)) {
        reparseStaleFile(fileNumber, baseArgs);
    } else {
        removeReferenceableItemsForFile(fileNumber);
    }
}
