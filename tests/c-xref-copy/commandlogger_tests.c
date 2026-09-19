#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "argumentsvector.h"
#include "commandlogger.h"
#include "fileio.mock"
#include "options.mock"


#if CGREEN_VERSION_MINOR < 5
#include "cgreen_capture_parameter.c"
#endif


static char *cwd = "currentdir";

Describe(CommandsLogger);
BeforeEach(CommandsLogger) {
    options.commandlog = "command log file";
    expect(openFile);
    expect(getCwd, will_set_contents_of_parameter(buffer, cwd, strlen(cwd)+1));
}
AfterEach(CommandsLogger) {}

Ensure(CommandsLogger, can_log_a_single_argument) {
    char *argv[] = {"command"};
    int size;
    int count;
    char *buffer;

    expect(writeFile,
           will_capture_parameter(size, size),
           will_capture_parameter(count, count),
           will_capture_parameter(buffer, buffer));
    always_expect(writeFile);

    ArgumentsVector args = {1, argv};
    logCommands(args);

    assert_that(size*count, is_equal_to(strlen(argv[0])));
    assert_that(buffer, is_equal_to_string("command"));
}

static char buffer[1000] = "";
static void concat_output(void *output_buffer) {
    strcat(buffer, *(char **)output_buffer);
}

// NOTE: these two tests will fail if Cgreen has a version below
// 1.6.1. This versions ensures that `with_side_effect()` executes
// *after* all other constraints.
Ensure(CommandsLogger, can_log_multiple_arguments) {
    char *argv[] = {"command", "arg1", "arg2"};
    char *output;

    always_expect(writeFile,
                  will_capture_parameter(buffer, output),
                  with_side_effect(&concat_output, &output));

    ArgumentsVector args = {3, argv};
    logCommands(args);

    assert_that(buffer, is_equal_to_string("command arg1 arg2\n"));
}

Ensure(CommandsLogger, can_handle_null_argv0) {
    char *argv[] = {NULL, "arg1", "arg2"};
    char *output;

    always_expect(writeFile,
                  will_capture_parameter(buffer, output),
                  with_side_effect(&concat_output, &output));

    ArgumentsVector args = {3, argv};
    logCommands(args);

    assert_that(buffer, is_equal_to_string("arg1 arg2\n"));
}

Ensure(CommandsLogger, can_remove_current_directory_from_argument) {
    char *argv[] = {NULL, "arg1", "currentdir/arg2"};
    char *output;

    always_expect(writeFile,
                  will_capture_parameter(buffer, output),
                  with_side_effect(&concat_output, &output));

    ArgumentsVector args = {3, argv};
    logCommands(args);

    assert_that(buffer, is_equal_to_string("arg1 arg2\n"));
}
