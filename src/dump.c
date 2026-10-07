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

typedef struct {
    int *numbers;
    int  count;
} FileNumbers;

/* Every file in the table, sorted by name, without the "no file" entry. The caller frees
 * the numbers. */
static FileNumbers collectFileNumbersByName(void) {
    FileNumbers files = {.numbers = NULL, .count = 0};
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        if (i == NO_FILE_NUMBER)
            continue;
        files.numbers = realloc(files.numbers, (files.count + 1) * sizeof(int));
        files.numbers[files.count++] = i;
    }
    qsort(files.numbers, files.count, sizeof(int), compareFileNames);
    return files;
}

static void dumpKnowledge(void) {
    FileNumbers files = collectFileNumbersByName();

    for (int i = 0; i < files.count; i++) {
        FileItem *fileItem = getFileItemWithFileNumber(files.numbers[i]);
        char      fact[MAX_PPC_RECORD_SIZE];
        snprintf(fact, sizeof(fact), "%s: %s", fileItem->name,
                 fileTimestampIsZero(fileItem->knowledgeTime) ? "no knowledge" : "knowledge");
        ppcGenRecord(PPC_INFORMATION, fact);
    }
    free(files.numbers);
}

void dumpTable(char *selection) {
    if (strcmp(selection, "knowledge") == 0)
        dumpKnowledge();
}
