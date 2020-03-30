#include <cgreen/cgreen.h>

#include "cxref.h"

/* Dependencies: */
#include "filedescriptor.mock"
#include "filetab.mock"
#include "olcxtab.mock"
#include "globals.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "refactory.mock"
#include "misc.mock"
#include "classh.mock"
#include "html.mock"
#include "complete.mock"
#include "caching.mock"
#include "options.mock"
#include "symbol.mock"
#include "main.mock"
#include "commons.mock"
#include "charbuf.mock"
#include "jsemact.mock"
#include "memory.mock"
#include "utils.mock"
#include "reftab.mock"


/* Needed: */
S_fileTab s_fileTab;

/* Hacks to resolve unresolved symbols... */
#include "c_parser.tab.h"
YYSTYPE cyylval;


Describe(CxRef);
BeforeEach(CxRef) {}
AfterEach(CxRef) {}


Ensure(CxRef, get_class_num_from_class_linkname_will_return_default_value_if_not_member) {
    int defaultValue = 14;

    expect(fileTabExists,
           when(fileName, is_equal_to_string(";name.class")),
           will_return(false));

    assert_that(getClassNumFromClassLinkName("name", defaultValue), is_equal_to(defaultValue));
}

Ensure(CxRef, get_class_num_from_class_linkname_will_return_filenumber_if_member) {
    int defaultValue = 14;
    int position = 42;

    expect(fileTabExists,
           when(fileName, is_equal_to_string(";name.class")),
           will_return(true));
    expect(fileTabLookup, will_return(position));

    assert_that(getClassNumFromClassLinkName("name", defaultValue), is_equal_to(position));
}
