#include <cgreen/cgreen.h>

#include "../filetab.h"

Describe(FileTab);
BeforeEach(FileTab) {}
AfterEach(FileTab) {}

Ensure(FileTab, can_fetch_a_stored_filename) {}
