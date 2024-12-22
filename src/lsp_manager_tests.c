#include <cgreen/cgreen.h>

#include "lsp_manager.h"


Describe(Lsp);
BeforeEach(Lsp) {}
AfterEach(Lsp) {}

Ensure(Lsp, handles_valid_initialize_request) {
    const char* valid_init = "{\"method\":\"initialize\",\"params\":{}}";
    assert_that(lsp_handle_initialize(valid_init), is_equal_to(1));
}

Ensure(Lsp, rejects_invalid_initialize_request) {
    const char* invalid_msg = "{\"method\":\"unknown\"}";
    assert_that(lsp_handle_initialize(invalid_msg), is_equal_to(0));
}

Ensure(Lsp, accepts_valid_json_message) {
    char valid_msg[] = "{\"json\":true}";
    assert_that(lsp_receive_message(valid_msg, strlen(valid_msg)),
                is_equal_to(1));
}

Ensure(Lsp, rejects_invalid_json_message) {
    char invalid_msg[] = "not json";
    assert_that(lsp_receive_message(invalid_msg, strlen(invalid_msg)),
                is_equal_to(0));
}
