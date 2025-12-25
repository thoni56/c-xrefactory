#include "search.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
#include "match.h"
#include "options.h"
#include "session.h"
#include "filetable.h"
#include "commons.h"
#include "misc.h"
#include "globals.h"
#include "ppc.h"


static bool matchNameIsLessThan(Match *m1, Match *m2) {
    return strcmp(m1->name, m2->name) < 0;
}

static void sortMatchList(Match **matches,
                          bool (*compareFunction)(Match *, Match *)) {
    LIST_MERGE_SORT(Match, *matches, compareFunction);
}

static void removeMatchesWithSameName(Match *matches) {
    for (Match *m = matches; m != NULL; m = m->next) {
        while (m->next != NULL && strcmp(m->name, m->next->name) == 0) {
            // O.K. remove redundant one
            Match *tmp = m->next;
            m->next = m->next->next;
            freeMatch(tmp);
        }
    }
}

#define maxOf(a, b) (((a) > (b)) ? (a) : (b))

void gotoSearchItem(int refn) {
    Match *match;

    assert(refn > 0);
    assert(sessionData.searchingStack.top);
    match = getMatchOnNthLine(sessionData.searchingStack.top->matches, refn);
    if (match != NULL) {
        if (positionsAreNotEqual(match->reference.position, noPosition)) {
            ppcGotoPosition(match->reference.position);
        } else {
            indicateNoReference();
        }
    } else {
        indicateNoReference();
    }
}
char *createSearchLine_static(char *name, int fileNumber, int *len1, int *len2) {
    static char line[2 * COMPLETION_STRING_SIZE];
    char file[TMP_STRING_SIZE];
    char dir[TMP_STRING_SIZE];
    int l1 = strlen(name);

    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    assert(fileItem->name);
    char *realFilename = getRealFileName_static(fileItem->name);
    int filenameLength = strlen(realFilename);
    int l2 = strmcpy(file, simpleFileName(realFilename)) - file;

    int directoryNameLength = filenameLength;
    strncpy(dir, realFilename, directoryNameLength + 1);

    *len1 = maxOf(*len1, l1);
    *len2 = maxOf(*len2, l2);

    if (options.searchKind == SEARCH_DEFINITIONS_SHORT || options.searchKind == SEARCH_FULL_SHORT) {
        sprintf(line, "%s", name);
    } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(line, 2 * COMPLETION_STRING_SIZE - 1, "%-*s :%-*s :%s", *len1, name, *len2, file, dir);
#pragma GCC diagnostic pop
    }
    return line; /* static! */
}

void compactSearchResultsShort(void) {
    sortMatchList(&sessionData.searchingStack.top->matches, matchNameIsLessThan);
    if (options.searchKind == SEARCH_DEFINITIONS_SHORT || options.searchKind == SEARCH_FULL_SHORT) {
        removeMatchesWithSameName(sessionData.searchingStack.top->matches);
    }
}
