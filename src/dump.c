#include "dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "filetable.h"
#include "ppc.h"
#include "protocol.h"
#include "timestamp.h"


static int compareFileNames(const void *a, const void *b) {
    return strcmp(getFileItemWithFileNumber(*(const int *)a)->name,
                  getFileItemWithFileNumber(*(const int *)b)->name);
}

/* Every file in the table, sorted by name, without the "no file" entry. The caller frees. */
static int *collectFileNumbersByName(int *count) {
    *count = 0;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1))
        if (i != NO_FILE_NUMBER)
            (*count)++;
    int *fileNumbers = malloc(*count * sizeof(int));
    int n = 0;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1))
        if (i != NO_FILE_NUMBER)
            fileNumbers[n++] = i;
    qsort(fileNumbers, *count, sizeof(int), compareFileNames);
    return fileNumbers;
}

static void dumpKnowledge(void) {
    int  count;
    int *fileNumbers = collectFileNumbersByName(&count);

    for (int i = 0; i < count; i++) {
        FileItem *fileItem = getFileItemWithFileNumber(fileNumbers[i]);
        char      fact[MAX_PPC_RECORD_SIZE];
        snprintf(fact, sizeof(fact), "%s: %s", fileItem->name,
                 fileTimestampIsZero(fileItem->knowledgeTime) ? "no knowledge" : "knowledge");
        ppcGenRecord(PPC_INFORMATION, fact);
    }
    free(fileNumbers);
}

void dumpTable(char *selection) {
    if (strcmp(selection, "knowledge") == 0)
        dumpKnowledge();
}
