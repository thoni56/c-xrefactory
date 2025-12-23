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

protected Completion *newCompletion(char *name, char *fullName, int lineCount, Visibility visibility, struct reference ref, struct referenceableItem sym);

protected void freeCompletion(Completion *completion);

Ensure(Completion, can_allocate_and_free_a_completion) {
    Reference ref = makeReference((Position){0,0,0}, UsageNone, NULL);
    ReferenceableItem item = makeReferenceableItem("", TypeInt, StorageDefault, AutoScope, VisibilityLocal, 0);
    Completion *c = newCompletion("", "", 0, VisibilityLocal, ref, item);
    freeCompletion(c);
}

Ensure(Completion, can_allocate_and_free_completions) {
    Reference ref;
    Completion *l = completionListPrepend(NULL, "", "", NULL, NULL, &ref, 0);
    l = completionListPrepend(l, "", "", NULL, NULL, &ref, 0);

    freeCompletions(l);
}
