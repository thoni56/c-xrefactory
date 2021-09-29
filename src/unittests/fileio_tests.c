#include <cgreen/cgreen.h>

#include "fileio.h"
#include "log.h"


Describe(Fileio);
BeforeEach(Fileio) {
    log_set_level(LOG_ERROR);
}
AfterEach(Fileio) {}

Ensure(Fileio, can_get_status_without_stat_reference) {
    fileStatus("fileio_tests.c", NULL);
}
