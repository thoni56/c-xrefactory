#include "search.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
#include "match.h"
#include "options.h"
#include "session.h"


static bool matchIsLessThan(Match *c1, Match *c2) {
    return strcmp(c1->name, c2->name) < 0;
}

static void sortMatchList(Match **matches,
                          bool (*compareFunction)(Match *, Match *)) {
    LIST_MERGE_SORT(Match, *matches, compareFunction);
}

static void tagSearchShortRemoveMultipleLines(Match *matches) {
    for (Match *m = matches; m != NULL; m = m->next) {
    again:
        if (m->next != NULL && strcmp(m->name, m->next->name) == 0) {
            // O.K. remove redundant one
            Match *tmp = m->next;
            m->next = m->next->next;
            freeMatch(tmp);
            goto again;          /* Again, but don't advance */
        }
    }
}

void tagSearchCompactShortResults(void) {
    sortMatchList(&sessionData.searchingStack.top->matches, matchIsLessThan);
    if (options.searchKind == SEARCH_DEFINITIONS_SHORT || options.searchKind == SEARCH_FULL_SHORT) {
        tagSearchShortRemoveMultipleLines(sessionData.searchingStack.top->matches);
    }
}
