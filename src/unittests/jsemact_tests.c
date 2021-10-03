#include <cgreen/cgreen.h>

#include "../jsemact.h"

#include "yylex.mock"
#include "globals.mock"
#include "options.mock"
#include "javafqttab.mock"
#include "filedescriptor.mock"
#include "filetable.mock"
#include "symboltable.mock"
#include "editor.mock"
#include "commons.mock"
#include "classfilereader.mock"
#include "java_parser.mock"
#include "parsers.mock"
#include "recyacc.mock"
#include "jslsemact.mock"
#include "memory.mock"
#include "type.mock"
#include "typemodifier.mock"
#include "extract.mock"
#include "semact.mock"
#include "cxref.mock"
#include "misc.mock"
#include "fileio.mock"
#include "jsltypetab.mock"
#include "classcaster.mock"


Describe(Jsemact);
BeforeEach(Jsemact) {}
AfterEach(Jsemact) {}


Ensure(Jsemact, can_run_empty_test) {
}
