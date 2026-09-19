#include "server.h"

/* Unittests for Server */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"

#include "browsermenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "complete.mock"
#include "completion.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "editorbuffer.mock"
#include "filedescriptor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "init.mock"
#include "lexer.mock"
#include "macroargumenttable.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "parsing.mock"
#include "ppc.mock"
#include "refactorings.mock"
#include "refactory.mock"
#include "reference.mock"
#include "referenceableitemtable.mock"
#include "session.mock"
#include "startup.mock"
#include "symboltable.mock"
#include "type.mock"
#include "yacc_parser.mock"
#include "yylex.mock"


Describe(Server);
BeforeEach(Server) {
    log_set_level(LOG_ERROR);
}
AfterEach(Server) {}

Ensure(Server, has_a_none_operation) {
    assert_that(OLO_NONE, is_equal_to(0));
    assert_that(operationNamesTable[OLO_NONE], is_equal_to_string("OLO_NONE"));
}
