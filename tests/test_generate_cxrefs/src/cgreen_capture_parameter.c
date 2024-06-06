// This file implements the Cgreen constraint "will_capture_parameter" as a custom
// constraint. Include this file in the ..._tests.c file to use it with tests before
// Cgreen 1.5 where it was introduced

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

/* We need a copy of ... */

static CgreenValue make_cgreen_pointer_value(void *pointer) {
    CgreenValue value = {CGREEN_POINTER, {0}, sizeof(intptr_t)};
    value.value.pointer_value = pointer;
    return value;
}

/* Start of custom Cgreen constraint... */

static void capture_parameter(Constraint *constraint, const char *function, CgreenValue actual,
                              const char *test_file, int test_line, TestReporter *reporter) {
    (void)function;
    (void)test_file;
    (void)test_line;
    (void)reporter;

    memmove(constraint->expected_value.value.pointer_value, &actual.value, constraint->size_of_expected_value);
}

static bool compare_true() { return true; }

static Constraint *create_capture_parameter_constraint(const char *parameter_name, void *capture_to, size_t size_to_capture) {
    Constraint* constraint = create_constraint();

    constraint->type = CGREEN_VALUE_COMPARER_CONSTRAINT;
    constraint->compare = &compare_true;
    constraint->execute = &capture_parameter;
    constraint->name = "capture parameter";
    constraint->expected_value = make_cgreen_pointer_value(capture_to);
    constraint->size_of_expected_value = size_to_capture;
    constraint->parameter_name = parameter_name;

    return constraint;
}

#define will_capture_parameter(parameter_name, local_variable) create_capture_parameter_constraint(#parameter_name, &local_variable, sizeof(local_variable))

/* End of custom Cgreen constraint */
