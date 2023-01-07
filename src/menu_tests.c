#include <cgreen/cgreen.h>

#include "menu.h"

#include "log.h"

#include "reference.mock"


Describe(Menu);
BeforeEach(Menu) {
    log_set_level(LOG_ERROR);
}
AfterEach(Menu) {}

Ensure(Menu, can_run_empty_test) {
}
