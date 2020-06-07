#include <cgreen/cgreen.h>

/* Must #include it since we are (for now) unittesting an internal macro (ScanInt) */
#include "cxfile.c"

#include "globals.mock"
#include "olcxtab.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "html.mock"
#include "reftab.mock"
#include "filetab.mock"
#include "options.mock"
#include "yylex.mock"
#include "lexer.mock"
#include "editor.mock"
#include "memory.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "fileio.mock"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_console_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
}
