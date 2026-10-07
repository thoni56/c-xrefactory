#include "freshness.h"

#include "editor.h"
#include "filetable.h"
#include "timestamp.h"


/* The filesystem stamps modification times from a coarse clock that lags the
 * system clock, so a write just after the knowledge time can get a time up to
 * a tick before it. Generous; a gap too wide only costs an extra parse
 * (ADR-0032) */
#define COARSE_CLOCK_GAP_IN_NANOSECONDS 10000000L

static bool changedAfter(FileTimestamp changeTime, FileTimestamp knowledgeTime) {
    return fileTimestampIsLessThan(fileTimestampMinus(knowledgeTime, COARSE_CLOCK_GAP_IN_NANOSECONDS),
                                   changeTime);
}

bool knowledgeIsOutOfDate(int fileNumber) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    if (fileTimestampIsZero(fileItem->knowledgeTime))
        return true;
    return changedAfter(editorFileModificationTime(fileItem->name), fileItem->knowledgeTime);
}
