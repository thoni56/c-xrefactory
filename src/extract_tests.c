#include <cgreen/cgreen.h>

#include "extract.h"

#include "log.h"

/* Dependencies: */
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "refactory.mock"
#include "misc.mock"
#include "classhierarchy.mock"
#include "complete.mock"
#include "caching.mock"
#include "options.mock"
#include "symbol.mock"
#include "main.mock"
#include "commons.mock"
#include "characterreader.mock"
#include "jsemact.mock"
#include "reftab.mock"
#include "symboltable.mock"
#include "cxref.mock"
#include "semact.mock"
#include "ppc.mock"



void myFatalError(int errCode, char *mess, int exitStatus) {
    fail_test("Fatal Error");
}

void myInternalCheckFail(char *expr, char *file, int line) {
    fail_test("Internal Check Failed");
}

void myError(int errCode, char *mess) {
    fail_test("Error");
}


Describe(Extract);
BeforeEach(Extract) {
    log_set_level(LOG_ERROR);
    memoryUseFunctionForError(myError);
    memoryUseFunctionForInternalCheckFail(myInternalCheckFail);
    memoryUseFunctionForFatalError(myFatalError);
    options.cxMemoryFactor = 2;
    if (!cxMemoryOverflowHandler(1))
        fail_test("cxMemoryOverflowHandler() returned false");
    dm_init(cxMemory, "cxMemory");
}
AfterEach(Extract) {}

/* Non-public function in extract module */
void addSymbolToSymRefList(ReferencesItemList **ll, ReferencesItem *s);

Ensure(Extract, can_concat_symRefItemList_when_null) {
    ReferencesItemList *lp = NULL;
    ReferencesItem s = {"s", 0, 0, 0, .references = NULL, NULL};

    addSymbolToSymRefList(&lp, &s);
    assert_that(lp->item, is_equal_to(&s));
}

Ensure(Extract, can_concat_symRefItemList_before_existing) {
    ReferencesItem s1 = {"s1", 0, 0, 0, .references = NULL, NULL};
    ReferencesItem s2 = {"s2", 0, 0, 0, .references = NULL, NULL};
    ReferencesItemList l = {&s1, NULL};
    ReferencesItemList *lp = &l;

    expect(getClassNumFromClassLinkName, times(4));
    expect(isSmallerOrEqClass, will_return(false), times(2));

    addSymbolToSymRefList(&lp, &s2);
    assert_that(lp->item, is_equal_to(&s2));
    assert_that(lp->next->item, is_equal_to(&s1));
}
