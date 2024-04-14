#include <cgreen/cgreen.h>

#include "complete.h"

#include "log.h"

#include "completion.mock"
#include "commons.mock"
#include "cxfile.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "jsemact.mock"
#include "misc.mock"
#include "options.mock"
#include "parsers.mock"
#include "ppc.mock"
#include "reftab.mock"
#include "semact.mock"
#include "symbol.mock"
#include "symboltable.mock"

/* Private method: */
int printJavaModifiers(char *buf, int *size, Access access);

Describe(Complete);
BeforeEach(Complete) {
    log_set_level(LOG_ERROR);
}
AfterEach(Complete) {}

Ensure(Complete, can_print_java_modifiers) {
    char buf[100];
    int  size = 100;

    printJavaModifiers(buf, &size, AccessPublic + AccessFinal);
    assert_that(buf, is_equal_to_string("public final "));
}
