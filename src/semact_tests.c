#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "semact.h"

#include "globals.mock"
#include "options.mock"
#include "caching.mock"
#include "filetable.mock"
#include "jsemact.mock"
#include "jslsemact.mock"
#include "classfilereader.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "yylex.mock"
#include "commons.mock"
#include "editor.mock"
#include "symboltable.mock"
#include "typemodifier.mock"
#include "symbol.mock"
#include "misc.mock"
#include "classcaster.mock"


Describe(Semact);
BeforeEach(Semact) {}
AfterEach(Semact) {}


Ensure(Semact, can_generate_message_for_no_such_field) {
    options.debug = true;
    expect(errorMessage,
           when(message, is_equal_to_string("Field/member 'field' not found")));
    noSuchFieldError("field");
}
