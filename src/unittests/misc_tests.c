#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "misc.h"

#include "globals.mock"
#include "caching.mock"
#include "filetab.mock"
#include "jsemact.mock"
#include "classfilereader.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "yylex.mock"
#include "memory.mock"
#include "commons.mock"
#include "editor.mock"


Describe(Misc);
BeforeEach(Misc) {}
AfterEach(Misc) {}


Ensure(Misc, can_generate_message_for_no_such_field) {
    s_opt.debug = true;
    expect(errorMessage,
           when(message, is_equal_to_string("Field/member 'field' not found")));
    noSuchFieldError("field");
}
