#include "freshness.h"

#include <stdlib.h>

#include "constants.h"
#include "editor.h"
#include "filetable.h"
#include "includegraph.h"
#include "log.h"
#include "timestamp.h"


struct prerequisites {
    IncludeGraph *includeGraph;
};

Prerequisites *collectPrerequisites(void) {
    Prerequisites *prerequisites = malloc(sizeof(Prerequisites));
    prerequisites->includeGraph = buildIncludeGraph();
    return prerequisites;
}

void freePrerequisites(Prerequisites *prerequisites) {
    freeIncludeGraph(prerequisites->includeGraph);
    free(prerequisites);
}

/* The filesystem stamps modification times from a coarse clock that lags the
 * system clock, so a write just after the knowledge time can get a time up to
 * a tick before it. Generous; a gap too wide only costs an extra parse
 * (ADR-0032) */
#define COARSE_CLOCK_GAP_IN_NANOSECONDS 10000000L

#define MAX_INCLUDE_CLOSURE 1000

/* Stamped as the coarse clock would, a gap back, so that knowledge built after
 * it is current and knowledge built before it is not */
FileTimestamp changeTimeNow(void) {
    return fileTimestampMinus(fileTimestampNow(), COARSE_CLOCK_GAP_IN_NANOSECONDS);
}

bool changedAfter(FileTimestamp changeTime, FileTimestamp readTime) {
    return fileTimestampIsLessThan(fileTimestampMinus(readTime, COARSE_CLOCK_GAP_IN_NANOSECONDS),
                                   changeTime);
}

/* A prerequisite that no longer exists has no modification time, and knowledge built
 * from it is out of date as well */
static bool prerequisiteChangedAfter(FileTimestamp changeTime, FileTimestamp knowledgeTime) {
    return fileTimestampIsZero(changeTime) || changedAfter(changeTime, knowledgeTime);
}

int collectIncludeClosureOf(Prerequisites *prerequisites, int fileNumber, int fileNumbers[],
                            int maxFileNumbers) {
    return collectIncludeClosure(prerequisites->includeGraph, fileNumber, fileNumbers, maxFileNumbers);
}

int collectIncluderClosureOf(Prerequisites *prerequisites, int fileNumber, int fileNumbers[],
                             int maxFileNumbers) {
    return collectIncluderClosure(prerequisites->includeGraph, fileNumber, fileNumbers, maxFileNumbers);
}

static OutOfDate whyOutOfDate(Prerequisites *prerequisites, int fileNumber, FileItem *fileItem) {
    if (fileTimestampIsZero(fileItem->knowledgeTime))
        return (OutOfDate){KNOWLEDGE_MISSING, fileNumber};
    if (prerequisiteChangedAfter(editorFileModificationTime(fileItem->name), fileItem->knowledgeTime))
        return (OutOfDate){CU_CHANGED, fileNumber};

    int closure[MAX_INCLUDE_CLOSURE];
    int count = collectIncludeClosure(prerequisites->includeGraph, fileNumber, closure, MAX_INCLUDE_CLOSURE);
    if (count > MAX_INCLUDE_CLOSURE)
        return (OutOfDate){CLOSURE_TOO_LARGE, fileNumber};
    for (int i = 0; i < count; i++) {
        if (changedAfter(getFileItemWithFileNumber(closure[i])->noticeTime, fileItem->knowledgeTime))
            return (OutOfDate){PREREQUISITE_CHANGED, closure[i]};
    }
    return (OutOfDate){KNOWLEDGE_CURRENT, fileNumber};
}

OutOfDate whyKnowledgeIsOutOfDate(Prerequisites *prerequisites, int fileNumber) {
    return whyOutOfDate(prerequisites, fileNumber, getFileItemWithFileNumber(fileNumber));
}

static void logOutOfDate(char *name, OutOfDate outOfDate) {
    switch (outOfDate.reason) {
    case KNOWLEDGE_CURRENT:
        break;
    case KNOWLEDGE_MISSING:
        log_info("'%s' is out of date: there is no knowledge of it", name);
        break;
    case CU_CHANGED:
        log_info("'%s' is out of date: it changed", name);
        break;
    case PREREQUISITE_CHANGED:
        log_info("'%s' is out of date: '%s' changed", name,
                 getFileItemWithFileNumber(outOfDate.fileNumber)->name);
        break;
    case CLOSURE_TOO_LARGE:
        log_info("'%s' is out of date: its include closure is too large to check", name);
        break;
    }
}

bool knowledgeIsOutOfDate(Prerequisites *prerequisites, int fileNumber) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    OutOfDate outOfDate = whyOutOfDate(prerequisites, fileNumber, fileItem);
    logOutOfDate(fileItem->name, outOfDate);
    return outOfDate.reason != KNOWLEDGE_CURRENT;
}
