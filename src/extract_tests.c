#include <cgreen/cgreen.h>

#include "extract.h"

#include "log.h"

/* Dependencies: */
#include "caching.mock"
#include "counters.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "commons.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "ppc.mock"
#include "refactory.mock"
#include "reftab.mock"
#include "semact.mock"
#include "symbol.mock"
#include "symboltable.mock"

void myFatalError(int errCode, char *mess, int exitStatus, char *file, int line) {
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
    setErrorHandlerForMemory(myError);
    setInternalCheckFailHandlerForMemory(myInternalCheckFail);
    setFatalErrorHandlerForMemory(myFatalError);
    options.cxMemoryFactor = 2;
    if (!cxMemoryOverflowHandler(1))
        fail_test("cxMemoryOverflowHandler() returned false");
    dm_init(cxMemory, "cxMemory");
}
AfterEach(Extract) {}


void addSymbolToSymRefList(ReferenceItemList **ll, ReferenceItem *s);

Ensure(Extract, can_concat_symRefItemList_when_null) {
    ReferenceItemList *lp = NULL;
    ReferenceItem      s  = {"s", 0, 0, 0, .references = NULL, NULL};

    addSymbolToSymRefList(&lp, &s);
    assert_that(lp->item, is_equal_to(&s));
}

Ensure(Extract, can_concat_symRefItemList_before_existing) {
    ReferenceItem      s1 = {"s1", 0, 0, 0, .references = NULL, NULL};
    ReferenceItem      s2 = {"s2", 0, 0, 0, .references = NULL, NULL};
    ReferenceItemList  l  = {&s1, NULL};
    ReferenceItemList *lp = &l;

    expect(getClassNumFromClassLinkName, times(4));
    expect(isSmallerOrEqClass, will_return(false), times(2));

    addSymbolToSymRefList(&lp, &s2);
    assert_that(lp->item, is_equal_to(&s2));
    assert_that(lp->next->item, is_equal_to(&s1));
}
