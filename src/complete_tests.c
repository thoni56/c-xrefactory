#include <cgreen/cgreen.h>

#include "complete.h"
#include "log.h"

#include "globals.mock"
#include "classfilereader.mock"
#include "options.mock"
#include "reftab.mock"
#include "parsers.mock"
#include "filetable.mock"
#include "symbol.mock"
#include "symboltable.mock"
#include "jsemact.mock"
#include "editor.mock"
#include "misc.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "semact.mock"
#include "ppc.mock"
#include "fileio.mock"



/* Private method: */
int printJavaModifiers(char *buf, int *size, Access access);


Describe(Complete);
BeforeEach(Complete) {
    log_set_level(LOG_ERROR);
}
AfterEach(Complete) {}

Ensure(Complete, can_print_java_modifiers) {
    char buf[100];
    int size = 100;

    printJavaModifiers(buf, &size, AccessPublic+AccessFinal);
    assert_that(buf, is_equal_to_string("public final "));
}
