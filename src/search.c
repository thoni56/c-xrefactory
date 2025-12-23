#include "search.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"
#include "match.h"
#include "options.h"
#include "session.h"


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

void tagSearchCompactShortResults(void) {
    sortMatchList(&sessionData.searchingStack.top->matches, matchNameIsLessThan);
    if (options.searchKind == SEARCH_DEFINITIONS_SHORT || options.searchKind == SEARCH_FULL_SHORT) {
        removeMatchesWithSameName(sessionData.searchingStack.top->matches);
    }
}
