#include <cgreen/cgreen.h>

#include "refactorings.h"

#include "log.h"


Describe(Xref);
BeforeEach(Xref) {
    log_set_level(LOG_ERROR);
}
AfterEach(Xref) {}

Ensure(Xref, can_set_and_clear_available_refactorings) {
    clearAvailableRefactorings();
    makeRefactoringAvailable(AVR_DEL_PARAMETER, "option");
    assert_that(isRefactoringAvailable(AVR_DEL_PARAMETER));
    assert_that(!isRefactoringAvailable(AVR_ADD_PARAMETER));
    assert_that(availableRefactoringOptionFor(AVR_DEL_PARAMETER), is_equal_to_string("option"));
}
