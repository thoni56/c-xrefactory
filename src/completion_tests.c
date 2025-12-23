#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "completion.h"

#include "head.h"               /* For 'protected' */
#include "type.h"

#include "commons.mock"
#include "filetable.mock"       /* For NO_FILE_NUMBER */
#include "options.mock"
#include "reference.mock"
#include "session.mock"
#include "symbol.mock"


Describe(Completion);
BeforeEach(Completion) {}
AfterEach(Completion) {}

protected Match *newCompletion(char *name, char *fullName, int lineCount, Visibility visibility, struct reference ref, struct referenceableItem sym);

protected void freeCompletion(Match *completion);

Ensure(Completion, can_allocate_and_free_a_completion) {
    Reference ref = makeReference((Position){0,0,0}, UsageNone, NULL);
    ReferenceableItem item = makeReferenceableItem("", TypeInt, StorageDefault, AutoScope, VisibilityLocal, 0);
    Match *c = newCompletion("", "", 0, VisibilityLocal, ref, item);
    freeCompletion(c);
}

Ensure(Completion, can_allocate_and_free_completions) {
    Reference ref;
    Match *l = prependToMatches(NULL, "", "", NULL, NULL, &ref, 0);
    l = prependToMatches(l, "", "", NULL, NULL, &ref, 0);

    freeMatches(l);
}
