#include <cgreen/cgreen.h>

#include "fileio.h"
#include "log.h"

Describe(Fileio);
BeforeEach(Fileio) {
    log_set_level(LOG_ERROR);
}
AfterEach(Fileio) {}

Ensure(Fileio, can_see_if_exists) {
    assert_that(exists("."));
    assert_that(exists("some file that do not exist"), is_false);
}

Ensure(Fileio, can_create_directory_even_if_exists) {
    createDirectory(".");
}
