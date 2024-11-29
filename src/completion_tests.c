#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "completion.h"

#include "head.h"               /* For 'protected' */
#include "type.h"

#include "reference.mock"
#include "symbol.mock"

#include "filetable.mock"       /* For NO_FILE_NUMBER */
#include "options.mock"         /* For 'options' */


Describe(Completion);
BeforeEach(Completion) {}
AfterEach(Completion) {}

protected Completion *newCompletion(char *name, char *fullName, int lineCount, ReferenceVisibility visibility,
                                    Type csymType, struct reference ref, struct referenceItem sym);

protected void olcxFreeCompletion(Completion *completion);

Ensure(Completion, can_allocate_and_free_a_completion) {
    Reference ref;
    ReferenceItem item;
    Completion *c = newCompletion("", "", 0, LocalVisibility, TypeInt, ref, item);
    olcxFreeCompletion(c);
}

Ensure(Completion, can_allocate_and_free_completions) {
    Reference ref;
    Completion *l = completionListPrepend(NULL, "", "", NULL, NULL, &ref, TypeInt, 0);
    l = completionListPrepend(l, "", "", NULL, NULL, &ref, TypeInt, 0);

    olcxFreeCompletions(l);
}
