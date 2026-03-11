#include "referencerefresh.h"

#include "commons.h"
#include "editor.h"
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

/* Ensure all files containing references to the given symbol have fresh
 * references. Finds which files have references, checks staleness, and
 * reparses stale ones. */
void ensureFreshReferences(ReferenceableItem *item, ArgumentsVector baseArgs) {
    /* Walk references, dedup by file number, check staleness, reparse */
    for (Reference *r = item->references; r != NULL; r = r->next) {
        int fileNumber = r->position.file;
        assert(fileNumber != NO_FILE_NUMBER);

        /* Skip if we already processed this file (earlier in the list) */
        bool alreadySeen = false;
        for (Reference *prev = item->references; prev != r; prev = prev->next) {
            if (prev->position.file == fileNumber) {
                alreadySeen = true;
                break;
            }
        }
        if (alreadySeen)
            continue;

        FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
        if (editorFileModificationTime(fileItem->name) != fileItem->lastParsedMtime) {
            reparseStaleFile(fileNumber, baseArgs);
        }
    }
}


void reparseStaleFile(int fileNumber, ArgumentsVector baseArgs) {
    removeReferenceableItemsForFile(fileNumber);
    parseFileWithFullInit(getFileItemWithFileNumber(fileNumber)->name, baseArgs);
}
