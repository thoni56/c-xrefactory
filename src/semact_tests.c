#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "semact.h"

#include "caching.mock"
#include "classcaster.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "jslsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "symbol.mock"
#include "symboltable.mock"
#include "typemodifier.mock"
#include "yylex.mock"

Describe(Semact);
BeforeEach(Semact) {}
AfterEach(Semact) {}

Ensure(Semact, can_generate_message_for_no_such_field) {
    options.debug = true;
    expect(errorMessage, when(message, is_equal_to_string("Field/member 'field' not found")));
    noSuchFieldError("field");
}
