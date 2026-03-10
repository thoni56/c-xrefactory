#include "referencerefresh.h"

#include "filetable.h"
#include "navigation.h"
#include "referenceableitemtable.h"
#include "filedescriptor.h"
#include "parsing.h"
#include "startup.h"
#include "referencerefresh.h"
#include "globals.h"


/* Parse a file using the full initializeFileProcessing machinery (project
 * discovery, options, checkpoint) with multi-pass support.  Saves and
 * restores REQUEST-level option fields that initOptions() inside
 * initializeFileProcessing would otherwise wipe. */
void parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs) {
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
