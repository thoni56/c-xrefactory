#include <cgreen/cgreen.h>

/* Need to include C file to test static functions */
#include "extract.c"

/* Dependencies: */
#include "filedescriptor.mock"
#include "filetab.mock"
#include "olcxtab.mock"
#include "globals.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "refactory.mock"
#include "misc.mock"
#include "classhierarchy.mock"
#include "html.mock"
#include "complete.mock"
#include "caching.mock"
#include "options.mock"
#include "symbol.mock"
#include "main.mock"
#include "commons.mock"
#include "characterreader.mock"
#include "jsemact.mock"
#include "utils.mock"
#include "reftab.mock"
#include "symboltable.mock"
#include "cxref.mock"
#include "semact.mock"


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
    memoryUseFunctionForError(myError);
    memoryUseFunctionForInternalCheckFail(myInternalCheckFail);
    memoryUseFunctionForFatalError(myFatalError);
    options.cxMemoryFactor = 2;
    if (!cxMemoryOverflowHandler(1))
        fail_test("cxMemoryOverflowHandler() returned 0");
    DM_INIT(cxMemory);
}
AfterEach(Extract) {}


Ensure(Extract, can_concat_symRefItemList_when_null) {
    SymbolReferenceItemList *lp = NULL;
    SymbolReferenceItem s = {"s", 0, 0, 0, {}, NULL, NULL};

    addSymbolToSymRefList(&lp, &s);
    assert_that(lp->d, is_equal_to(&s));
}

Ensure(Extract, can_concat_symRefItemList_before_existing) {
    SymbolReferenceItem s1 = {"s1", 0, 0, 0, {}, NULL, NULL};
    SymbolReferenceItem s2 = {"s2", 0, 0, 0, {}, NULL, NULL};
    SymbolReferenceItemList l = {&s1, NULL};
    SymbolReferenceItemList *lp = &l;

    expect(getClassNumFromClassLinkName, times(4));
    expect(isSmallerOrEqClass, will_return(false), times(2));

    addSymbolToSymRefList(&lp, &s2);
    assert_that(lp->d, is_equal_to(&s2));
    assert_that(lp->next->d, is_equal_to(&s1));
}
