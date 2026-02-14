#include "session.h"

/* Unittests */

#include <cgreen/cgreen.h>

#include "log.h"

#include "browsingmenu.mock"
#include "commons.mock"
#include "completion.mock"
#include "filetable.mock"
#include "globals.mock"
#include "match.mock"
#include "options.mock"
#include "reference.mock"


Describe(Session);
BeforeEach(Session) {
    log_set_level(LOG_ERROR);
}
AfterEach(Session) {}

Ensure(Session, can_run_an_empty_test) {
}

Ensure(Session, getSessionEntryForOperation_returns_completionStack_top_for_OP_COMPLETION) {
    SessionStackEntry entry;
    sessionData.completionStack.top = &entry;

    SessionStackEntry *result = getSessionEntryForOperation(OP_COMPLETION);

    assert_that(result, is_equal_to(&entry));
}

Ensure(Session, getSessionEntryForOperation_returns_NULL_when_completionStack_top_is_NULL) {
    sessionData.completionStack.top = NULL;

    SessionStackEntry *result = getSessionEntryForOperation(OP_COMPLETION);

    assert_that(result, is_null);
}

Ensure(Session, getSessionEntryForOperation_returns_browsingStack_top_for_browsing_operation) {
    SessionStackEntry entry;
    sessionData.browsingStack.top = &entry;

    SessionStackEntry *result = getSessionEntryForOperation(OP_BROWSE_PUSH);

    assert_that(result, is_equal_to(&entry));
}
