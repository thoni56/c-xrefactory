#include "referencerefresh.h"

#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "editor.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "parsing.h"
#include "referenceableitemtable.h"
#include "startup.h"


/* Parse a file using the full initializeFileProcessing machinery (project
 * discovery, options, checkpoint) with multi-pass support.  Saves and
 * restores REQUEST-level option fields that initOptions() inside
 * initializeFileProcessing would otherwise wipe. */
static bool parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs) {
    bool parsed = false;
    int savedCursorOffset = options.cursorOffset;
    int savedIndex = options.index;
    ServerOperation savedServerOperation = options.serverOperation;
    /* A string option lives in the options' own memory, which the reset frees, so it is
     * copied out and allocated again. A stopgap until the options partition: every other
     * REQUEST option is reset here too. */
    char *savedDumpSelection = options.dumpSelection != NULL ? strdup(options.dumpSelection) : NULL;

    inputFileName = fileName;
    ArgumentsVector emptyArgs = {.argc = 0, .argv = NULL};
    maxPasses = 1;
    silenceMessages();
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        if (initializeFileProcessing(baseArgs, emptyArgs)) {
            options.cursorOffset = NO_CURSOR_OFFSET;
            parseToCreateReferences(inputFileName);
            parsed = true;
            closeCharacterBuffer(&currentFile.characterBuffer);
            currentFile.characterBuffer.file = stdin;
        }
        currentFile.characterBuffer.isAtEOF = false;
    }
    resumeMessages();

    options.cursorOffset = savedCursorOffset;
    options.index = savedIndex;
    options.serverOperation = savedServerOperation;
    if (savedDumpSelection != NULL) {
        allocateStringForOption(&options.dumpSelection, savedDumpSelection);
        free(savedDumpSelection);
    }
    return parsed;
}

void buildKnowledgeOfCU(int fileNumber, ArgumentsVector baseArgs) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    char *fileName = fileItem->name;
    if (!editorFileExists(fileName)) {
        markFileAsDeleted(fileNumber);
        return;
    }
    removeReferenceableItemsForFile(fileNumber);
    /* Taken before the content is read, so an edit during the parse makes the
     * knowledge out of date. */
    FileTimestamp knowledgeTime = fileTimestampNow();
    bool parsed = parseFileWithFullInit(fileName, baseArgs);
    fileItem->lastParsedMtime = editorFileModificationTime(fileName);
    if (parsed)
        fileItem->knowledgeTime = knowledgeTime;
}
