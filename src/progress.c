#include "progress.h"

#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#include "protocol.h"

int progressOffset=0;
int progressFactor=1;

static void writeProgressInformation(int progress) {
    static int      lastprogress;
    static time_t   timeZero;
    static bool     dialogDisplayed = false;
    static bool     initialCall = true;
    time_t          currentTime;

    if (progress == 0 || initialCall) {
        initialCall = false;
        dialogDisplayed = false;
        lastprogress = 0;
        timeZero = time(NULL);
    } else {
        if (progress <= lastprogress)
            return;
    }
    currentTime = time(NULL);
    // write progress only if it seems to be longer than 3 sec
    if (dialogDisplayed
        || (progress == 0 && currentTime-timeZero > 1)
        || (progress != 0 && currentTime-timeZero >= 1 && 100*((double)currentTime-timeZero)/progress > 3)
        ) {
        if (!dialogDisplayed) {
            // display progress bar
            fprintf(stdout, "<%s>progress 0%%</%s>\n", PPC_PROGRESS, PPC_PROGRESS);
            dialogDisplayed = true;
        }
        fprintf(stdout, "<%s>progress %d%%</%s>\n", PPC_PROGRESS, progress, PPC_PROGRESS);
        fflush(stdout);
        lastprogress = progress;
    }
}

void writeRelativeProgress(int progress) {
    writeProgressInformation((100*progressOffset + progress)/progressFactor);
    if (progress==100)
        progressOffset++;
}
