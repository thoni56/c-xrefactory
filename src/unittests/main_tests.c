#include <cgreen/cgreen.h>

#include "main.h"

#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "filedescriptor.mock"
#include "characterreader.mock"
#include "reftab.mock"
#include "filetable.mock"
#include "macroargumenttable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"

#include "parsers.mock"
#include "c_parser.mock"
#include "yacc_parser.mock"
#include "java_parser.mock"

#include "symboltable.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "editor.mock"
#include "jslsemact.mock"
#include "caching.mock"
#include "ppc.mock"
#include "refactory.mock"
#include "olcxtab.mock"
#include "jsemact.mock"
#include "yylex.mock"
#include "javafqttab.mock"
#include "complete.mock"
#include "type.mock"
#include "recyacc.mock"
#include "init.mock"
#include "utils.mock"
#include "lexer.mock"


Describe(Main);
BeforeEach(Main) {
    log_set_level(LOG_ERROR);
}
AfterEach(Main) {}

Ensure(Main, can_run_empty_test) {
}
