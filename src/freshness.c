#include "freshness.h"

#include <stdlib.h>

#include "constants.h"
#include "editor.h"
#include "filetable.h"
#include "includegraph.h"
#include "log.h"
#include "timestamp.h"


typedef struct {
    bool          known;
    FileTimestamp time;
} ChangeTime;

struct inputs {
    IncludeGraph *includeGraph;
    ChangeTime   *changeTimes;   /* By file number, each asked for once per request */
};

Inputs *collectInputs(void) {
    Inputs *inputs = malloc(sizeof(Inputs));
    inputs->includeGraph = buildIncludeGraph();
    inputs->changeTimes = calloc(MAX_FILES, sizeof(ChangeTime));
    return inputs;
}

void freeInputs(Inputs *inputs) {
    freeIncludeGraph(inputs->includeGraph);
    free(inputs->changeTimes);
    free(inputs);
}

/* A file that no longer exists has a zero time, which is remembered like any other */
static FileTimestamp changeTimeOf(Inputs *inputs, int fileNumber) {
    ChangeTime *changeTime = &inputs->changeTimes[fileNumber];
    if (!changeTime->known) {
        changeTime->time = editorFileModificationTime(getFileItemWithFileNumber(fileNumber)->name);
        changeTime->known = true;
    }
    return changeTime->time;
}

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

/* An input that no longer exists has no modification time, and the knowledge
 * of it is out of date as well */
static bool inputChangedAfter(FileTimestamp changeTime, FileTimestamp knowledgeTime) {
    return fileTimestampIsZero(changeTime) || changedAfter(changeTime, knowledgeTime);
}

bool changedAfterKnowledgeOf(Inputs *inputs, int fileNumber, int cuFileNumber) {
    return inputChangedAfter(changeTimeOf(inputs, fileNumber),
                             getFileItemWithFileNumber(cuFileNumber)->knowledgeTime);
}

int collectIncludeClosureOf(Inputs *inputs, int fileNumber, int fileNumbers[],
                            int maxFileNumbers) {
    return collectIncludeClosure(inputs->includeGraph, fileNumber, fileNumbers, maxFileNumbers);
}

static OutOfDate whyOutOfDate(Inputs *inputs, int fileNumber, FileItem *fileItem) {
    if (fileTimestampIsZero(fileItem->knowledgeTime))
        return (OutOfDate){KNOWLEDGE_MISSING, fileNumber};
    if (inputChangedAfter(editorFileModificationTime(fileItem->name), fileItem->knowledgeTime))
        return (OutOfDate){CU_CHANGED, fileNumber};

    int closure[MAX_INCLUDE_CLOSURE];
    int count = collectIncludeClosure(inputs->includeGraph, fileNumber, closure, MAX_INCLUDE_CLOSURE);
    if (count > MAX_INCLUDE_CLOSURE)
        return (OutOfDate){CLOSURE_TOO_LARGE, fileNumber};
    for (int i = 0; i < count; i++) {
        if (getFileItemWithFileNumber(closure[i])->isLeftBehind)
            return (OutOfDate){INPUT_LEFT_BEHIND, closure[i]};
        if (inputChangedAfter(changeTimeOf(inputs, closure[i]), fileItem->knowledgeTime))
            return (OutOfDate){INPUT_CHANGED, closure[i]};
    }
    return (OutOfDate){KNOWLEDGE_CURRENT, fileNumber};
}

OutOfDate whyKnowledgeIsOutOfDate(Inputs *inputs, int fileNumber) {
    return whyOutOfDate(inputs, fileNumber, getFileItemWithFileNumber(fileNumber));
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
    case INPUT_CHANGED:
        log_info("'%s' is out of date: '%s' changed", name,
                 getFileItemWithFileNumber(outOfDate.fileNumber)->name);
        break;
    case INPUT_LEFT_BEHIND:
        log_info("'%s' is out of date: '%s' is left behind", name,
                 getFileItemWithFileNumber(outOfDate.fileNumber)->name);
        break;
    case CLOSURE_TOO_LARGE:
        log_info("'%s' is out of date: its include closure is too large to check", name);
        break;
    }
}

bool knowledgeIsOutOfDate(Inputs *inputs, int fileNumber) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    OutOfDate outOfDate = whyOutOfDate(inputs, fileNumber, fileItem);
    logOutOfDate(fileItem->name, outOfDate);
    return outOfDate.reason != KNOWLEDGE_CURRENT;
}
