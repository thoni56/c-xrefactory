#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "settings_handler.h"

#include "log.h"
#include "proto.h"

#include "fileio.mock"


Describe(SettingsHandler);
BeforeEach(SettingsHandler) {
    log_set_level(LOG_ERROR);
}
AfterEach(SettingsHandler) {}

Ensure(SettingsHandler, will_return_empty_filename_if_no_settings_file_is_found) {
    char project_name[PATH_MAX];

    expect(getCwd, will_set_contents_of_parameter(buffer, "/", 2));
    expect(fileExists, when(fullPath, is_equal_to_string("/.c-xrefrc")),
           will_return(false));

    assert_that(find_project_settings(project_name), is_equal_to(RESULT_NOT_FOUND));
    assert_that(project_name, is_equal_to_string(""));
}

Ensure(SettingsHandler, will_return_name_of_settings_file_in_current_directory) {
    char project_name[PATH_MAX];

    expect(getCwd, will_set_contents_of_parameter(buffer, "/home/cxref", 12));
    expect(fileExists, when(fullPath, is_equal_to_string("/home/cxref/.c-xrefrc")),
           will_return(true));

    assert_that(find_project_settings(project_name), is_equal_to(RESULT_OK));
    assert_that(project_name, is_equal_to_string("/home/cxref"));
}

Ensure(SettingsHandler, will_return_name_of_settings_file_in_parent_directory) {
    char project_name[PATH_MAX];

    expect(getCwd, will_set_contents_of_parameter(buffer, "/home/cxref", 12));
    expect(fileExists, when(fullPath, is_equal_to_string("/home/cxref/.c-xrefrc")),
           will_return(false));
    expect(fileExists, when(fullPath, is_equal_to_string("/home/.c-xrefrc")),
           will_return(true));

    assert_that(find_project_settings(project_name), is_equal_to(RESULT_OK));
    assert_that(project_name, is_equal_to_string("/home"));
}
