#include <cgreen/cgreen.h>

#include "jsemact.h"

#include "classcaster.mock"
#include "classfilereader.mock"
#include "commons.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "extract.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "java_parser.mock"
#include "javafqttab.mock"
#include "jslsemact.mock"
#include "jsltypetab.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "recyacc.mock"
#include "semact.mock"
#include "symboltable.mock"
#include "type.mock"
#include "typemodifier.mock"
#include "yylex.mock"

Describe(Jsemact);
BeforeEach(Jsemact) {}
AfterEach(Jsemact) {}

Ensure(Jsemact, can_run_empty_test) {}
