#include <cgreen/cgreen.h>
#include <cgreen/unit.h>

#include "completion.h"

#include "filetable.mock"
#include "cxfile.mock"
#include "options.mock"
#include "reference.mock"
#include "symbol.mock"


Describe(Completion);
BeforeEach(Completion) {}
AfterEach(Completion) {}

protected Completion *newCompletion(char *name, char *fullName, int lineCount, ReferenceCategory category,
                                    Type csymType, struct reference ref, struct referenceItem sym);

protected void olcxFreeCompletion(Completion *completion);

Ensure(Completion, can_allocate_and_free_completions) {
    Reference ref;
    ReferenceItem item;
    Completion *c = newCompletion("", "", 0, CategoryLocal, TypeInt, ref, item);
    olcxFreeCompletion(c);
}
