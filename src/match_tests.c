#include "match.h"

#include <cgreen/cgreen.h>
#include <cgreen/unit.h>


#include "head.h"               /* For 'protected' */
#include "type.h"

#include "commons.mock"
#include "filetable.mock"       /* For NO_FILE_NUMBER */
#include "options.mock"
#include "reference.mock"
#include "session.mock"
#include "symbol.mock"


Describe(Match);
BeforeEach(Match) {}
AfterEach(Match) {}

protected Match *newMatch(char *name, char *fullName, int lineCount, Visibility visibility, struct reference ref,
                          struct referenceableItem sym);

protected void freeMatch(Match *match);

Ensure(Match, can_allocate_and_free_a_match) {
    Reference ref = makeReference((Position){0,0,0}, UsageNone, NULL);
    ReferenceableItem item = makeReferenceableItem("", TypeInt, StorageDefault, AutoScope, VisibilityLocal, 0);
    Match *c = newMatch("", "", 0, VisibilityLocal, ref, item);
    freeMatch(c);
}

Ensure(Match, can_allocate_and_free_completions) {
    Reference ref;
    Match *l = prependToMatches(NULL, "", "", NULL, NULL, &ref, 0);
    l = prependToMatches(l, "", "", NULL, NULL, &ref, 0);

    freeMatches(l);
}
