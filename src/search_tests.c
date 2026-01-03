#include "proto.h"
#include "search.h"

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "log.h"
#include "session.h"

#include "browsermenu.mock"
#include "commons.mock"
#include "completion.mock"
#include "globals.mock"
#include "match.mock"
#include "options.mock"
#include "reference.mock"
#include "misc.mock"
#include "filetable.mock"
#include "ppc.mock"


Describe(Search);
BeforeEach(Search) {
    log_set_level(LOG_ERROR);
}
AfterEach(Search) {}

protected SessionStackEntry *newEmptySessionStackEntry(void);

static Match *newMatch(char *name) {
    Match *match = (Match *)malloc(sizeof(Match));
    match->next = NULL;
    match->name = name;
    return match;
}

Ensure(Search, can_compact_empty_search_results) {
    options.searchKind = SEARCH_DEFINITIONS_SHORT;
    sessionData.searchingStack.top = newEmptySessionStackEntry();

    never_expect(freeMatch);    /* No match should be freed, no matches */

    compactSearchResultsShort();
}

Ensure(Search, can_compact_search_results_with_single_match) {
    options.searchKind = SEARCH_DEFINITIONS_SHORT;
    sessionData.searchingStack.top = newEmptySessionStackEntry();
    sessionData.searchingStack.top->matches = newMatch("a");

    never_expect(freeMatch);    /* No match should be freed, no duplicates */

    compactSearchResultsShort();
}

Ensure(Search, can_compact_search_results_with_many_equal_matches) {
    options.searchKind = SEARCH_DEFINITIONS_SHORT;
    sessionData.searchingStack.top = newEmptySessionStackEntry();
    sessionData.searchingStack.top->matches = newMatch("a");
    sessionData.searchingStack.top->matches->next = newMatch("a");
    sessionData.searchingStack.top->matches->next->next = newMatch("a");

    expect(freeMatch, times(2)); /* Two duplicated matches should be freed */

    compactSearchResultsShort();
}
