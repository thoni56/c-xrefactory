#include "progress.h"

#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#include "protocol.h"

int progressOffset=0;
int progressFactor=1;

static const char *messageFormat = NULL;
static bool        dialogDisplayed = false;
static struct timespec timeZero;
static struct timespec lastEmitTime;

static double elapsedSeconds(struct timespec *from, struct timespec *to) {
    return (to->tv_sec - from->tv_sec) + (to->tv_nsec - from->tv_nsec) / 1e9;
}

void initProgress(const char *format) {
    messageFormat = format;
    dialogDisplayed = false;
    clock_gettime(CLOCK_MONOTONIC, &timeZero);
    lastEmitTime = timeZero;
}

void writeProgressInformation(int value) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    const char *format = messageFormat ? messageFormat : "progress %d%%";

    if (!dialogDisplayed) {
        if (elapsedSeconds(&timeZero, &now) <= 1.0)
            return;
        dialogDisplayed = true;
    } else {
        if (elapsedSeconds(&lastEmitTime, &now) < 0.2)
            return;
    }

    fprintf(stdout, "<%s>", PPC_PROGRESS);
    fprintf(stdout, format, value);
    fprintf(stdout, "</%s>\n", PPC_PROGRESS);
    fflush(stdout);
    lastEmitTime = now;
}

void writeRelativeProgress(int progress) {
    if (progress == 0)
        initProgress(NULL);
    writeProgressInformation((100*progressOffset + progress)/progressFactor);
    if (progress==100)
        progressOffset++;
}
