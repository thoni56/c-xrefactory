#include <cgreen/cgreen.h>

#include "server.h"

#include "log.h"

#include "characterreader.mock"
#include "commons.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "macroargumenttable.mock"
#include "memory.h"
#include "misc.mock"
#include "options.mock"
#include "reftab.mock"

#include "c_parser.mock"
#include "java_parser.mock"
#include "parsers.mock"
#include "yacc_parser.mock"

#include "caching.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "init.mock"
#include "javafqttab.mock"
#include "jsemact.mock"
#include "jslsemact.mock"
#include "lexer.mock"
#include "main.mock"
#include "ppc.mock"
#include "recyacc.mock"
#include "refactory.mock"
#include "symboltable.mock"
#include "type.mock"
#include "yylex.mock"

Describe(Server);
BeforeEach(Server) {
    log_set_level(LOG_ERROR);
}
AfterEach(Server) {}

Ensure(Server, can_run_empty_test) {}
