#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "commandlogger.h"
#include "fileio.mock"
#include "options.mock"


#if CGREEN_VERSION_MINOR < 5
#include "cgreen_capture_parameter.c"
#endif


Describe(CommandsLogger);
BeforeEach(CommandsLogger) {
    options.commandlog = true;
}
AfterEach(CommandsLogger) {}

Ensure(CommandsLogger, can_log_a_single_argument) {
    char *argv[] = {"command"};
    int size;
    int count;
    char *buffer;

    expect(openFile);
    expect(writeFile,
           will_capture_parameter(size, size),
           will_capture_parameter(count, count),
           will_capture_parameter(buffer, buffer));
    always_expect(writeFile);

    logCommands(1, argv);

    assert_that(size*count, is_equal_to(strlen(argv[0])));
    assert_that(buffer, is_equal_to_string("command"));
}

static char buffer[1000] = "";
static void concat_output(void *string) {
    strcat(buffer, *(char **)string);
}

Ensure(CommandsLogger, can_log_multiple_arguments) {
    char *argv[] = {"command", "arg1", "arg2"};
    char *output;

    expect(openFile);
    always_expect(writeFile,
                  will_capture_parameter(buffer, output),
                  with_side_effect(&concat_output, &output));

    logCommands(3, argv);

    assert_that(buffer, is_equal_to_string("command arg1 arg2\n"));
}

Ensure(CommandsLogger, can_handle_null_argv0) {
    char *argv[] = {NULL, "arg1", "arg2"};
    char *output;

    expect(openFile);
    always_expect(writeFile,
                  will_capture_parameter(buffer, output),
                  with_side_effect(&concat_output, &output));

    logCommands(3, argv);

    assert_that(buffer, is_equal_to_string("arg1 arg2\n"));
}
