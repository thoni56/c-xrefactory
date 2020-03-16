#include <cgreen/cgreen.h>

#include "cxref.h"

/* Dependencies: */
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


Ensure(CxRef, can_get_class_num_from_class_linkname) {
}
