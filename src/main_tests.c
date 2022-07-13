#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "main.h"

#include "log.h"

#include "c_parser.mock"
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
#include "java_parser.mock"
#include "javafqttab.mock"
#include "jsemact.mock"
#include "jslsemact.mock"
#include "lexer.mock"
#include "macroargumenttable.mock"
#include "memory.h"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "recyacc.mock"
#include "refactory.mock"
#include "reftab.mock"
#include "server.mock"
#include "symboltable.mock"
#include "type.mock"
#include "yacc_parser.mock"
#include "yylex.mock"

Describe(Main);
BeforeEach(Main) {
    log_set_level(LOG_ERROR);
}
AfterEach(Main) {}

Ensure(Main, mainCallXref_without_input_files_gives_error_message) {
    expect(getNextExistingFileIndex, will_return(-1));
    expect(errorMessage);

    cxMemoryOverflowHandler(0); /* Implicitly allocate and init cxMemory */

    mainCallXref(0, NULL);
}

xEnsure(Main, can_handle_cxMemory_overflow) {
    cxMemoryOverflowHandler(0);            /* Implicitly allocate and init cxMemory */
    cxMemory->index = cxMemory->size - 10; /* Simulate memory almost out */

    mainCallXref(0, NULL);
}
