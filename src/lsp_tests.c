#include <cgreen/cgreen.h>

#include "lsp.h"


Describe(Lsp);
BeforeEach(Lsp) {}
AfterEach(Lsp) {}

Ensure(Lsp, returns_true_when_lsp_option_is_in_argv) {}

Ensure(Lsp, returns_false_when_lsp_option_is_not_in_argv) {}
