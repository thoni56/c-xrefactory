// This file implements a custom Cgreen constraint "is_equal_to_json" which compares two
// JSON structures and print them out as strings for easy comparison. Include this file
// in the ..._tests.c file to use it with tests

#include <cgreen/cgreen.h>
#include <cgreen/cgreen_value.h>
#include <cgreen/constraint.h>
#include <cgreen/mocks.h>

/* We obvious need */
#include "json_utils.h"

/* Some non-public Cgreen functions */
extern CgreenValue make_cgreen_pointer_value(void *pointer);
extern void destroy_empty_constraint(Constraint *constraint);

/* And a copy of */
static Constraint *create_constraint_expecting(CgreenValue expected_value, const char *expected_value_name) {
    Constraint *constraint = create_constraint();

    constraint->expected_value = expected_value;
    constraint->expected_value_name = strdup(expected_value_name);

    return constraint;
}


/* Start of custom Cgreen constraint... */

static bool compare_want_json(Constraint *constraint, CgreenValue actual) {
    return json_equals((JSON*)actual.value.pointer_value, (JSON*)constraint->expected_value.value.pointer_value);
}

char *failure_message_for_json(Constraint *constraint, const char *actual_string,
                                      intptr_t actual_value) {

    static const char constraint_as_string_format[] = "Expected [%s] to [%s] [%s]";

    char *expected_value_as_string = print_json((JSON*)constraint->expected_value.value.pointer_value);
    char *actual_value_as_string = print_json((JSON*)actual_value);

    int message_length = strlen(constraint_as_string_format) + strlen(actual_string) +
        strlen(constraint->name) + strlen(constraint->expected_value_name) +
        strlen(constraint->expected_value_message) + strlen(expected_value_as_string) +
        strlen(constraint->actual_value_message) + strlen(actual_value_as_string);
    char *message = malloc(message_length + 1);

    /* Expected [name] to [equal to json] [name] */
    snprintf(message, message_length, constraint_as_string_format, actual_string, constraint->name, constraint->expected_value_name);
    snprintf(message+strlen(message), message_length-strlen(message), constraint->actual_value_message, actual_value_as_string);
    snprintf(message+strlen(message), message_length-strlen(message), constraint->expected_value_message, expected_value_as_string);

    return message;
}

static Constraint *create_equal_to_json_constraint(JSON *expected_value, const char *expected_value_name) {
    Constraint* constraint = create_constraint_expecting(make_cgreen_pointer_value(expected_value),
                                                                                   expected_value_name);

    constraint->type = CGREEN_STRING_COMPARER_CONSTRAINT;
    constraint->compare = &compare_want_json;
    constraint->execute = &test_want;
    constraint->failure_message = &failure_message_for_json;
    constraint->name = "equal to json";
    constraint->destroy = &destroy_empty_constraint;
    constraint->actual_value_message = "\n\t\tactual value:\t\t\t[%s]\n";
    constraint->expected_value_message = "\t\texpected to equal:\t\t[%s]";

    return constraint;
}

#define is_equal_to_json(json) create_equal_to_json_constraint(json, #json)

/* End of custom Cgreen constraint */
