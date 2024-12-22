#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "main.h"

#include "log.h"

#include "caching.mock"
#include "counters.mock"
#include "characterreader.mock"
#include "commandlogger.mock"
#include "commons.mock"
#include "complete.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "init.mock"
#include "lexem.mock"
#include "lexer.mock"
#include "lsp.mock"
#include "macroargumenttable.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "refactory.mock"
#include "reftab.mock"
#include "server.mock"
#include "symboltable.mock"
#include "type.mock"
#include "xref.mock"
#include "yylex.mock"


Describe(Main);
BeforeEach(Main) {
    log_set_level(LOG_ERROR);
}
AfterEach(Main) {}

Ensure(Main, can_run_empty_test) {
    pass_test();
}
