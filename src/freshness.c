#include "freshness.h"

#include "editor.h"
#include "filetable.h"
#include "includegraph.h"
#include "timestamp.h"


/* The filesystem stamps modification times from a coarse clock that lags the
 * system clock, so a write just after the knowledge time can get a time up to
 * a tick before it. Generous; a gap too wide only costs an extra parse
 * (ADR-0032) */
#define COARSE_CLOCK_GAP_IN_NANOSECONDS 10000000L

#define MAX_INCLUDE_CLOSURE 1000

static bool changedAfter(FileTimestamp changeTime, FileTimestamp knowledgeTime) {
    return fileTimestampIsLessThan(fileTimestampMinus(knowledgeTime, COARSE_CLOCK_GAP_IN_NANOSECONDS),
                                   changeTime);
}

static bool fileChangedAfter(FileItem *input, FileTimestamp knowledgeTime) {
    return changedAfter(editorFileModificationTime(input->name), knowledgeTime);
}

bool knowledgeIsOutOfDate(int fileNumber) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    if (fileTimestampIsZero(fileItem->knowledgeTime))
        return true;
    if (fileChangedAfter(fileItem, fileItem->knowledgeTime))
        return true;

    int closure[MAX_INCLUDE_CLOSURE];
    int count = collectIncludeClosure(fileNumber, closure, MAX_INCLUDE_CLOSURE);
    if (count > MAX_INCLUDE_CLOSURE)
        return true;
    for (int i = 0; i < count; i++) {
        FileItem *includedItem = getFileItemWithFileNumber(closure[i]);
        if (fileChangedAfter(includedItem, fileItem->knowledgeTime))
            return true;
    }
    return false;
}
