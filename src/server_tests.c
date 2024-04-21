#include <cgreen/cgreen.h>

#include "server.h"

#include "log.h"


#include "caching.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "init.mock"
#include "lexer.mock"
#include "macroargumenttable.mock"
#include "main.mock"
#include "menu.mock"
#include "memory.h"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "refactorings.mock"
#include "refactory.mock"
#include "reference.mock"
#include "reftab.mock"
#include "symboltable.mock"
#include "type.mock"
#include "yacc_parser.mock"
#include "yylex.mock"


Describe(Server);
BeforeEach(Server) {
    log_set_level(LOG_ERROR);
}
AfterEach(Server) {}

Ensure(Server, can_run_empty_test) {}
