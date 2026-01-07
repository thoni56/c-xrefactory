#include "parsers.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"

#include "c_parser.mock"
#include "yacc_parser.mock"
#include "parsing.mock"


Describe(Parsers);
BeforeEach(Parsers) {
    log_set_level(LOG_ERROR);
}
AfterEach(Parsers) {}

Ensure(Parsers, can_run_empty_test) {
}
