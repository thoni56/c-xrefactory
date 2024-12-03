#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/unit.h>

#include "list.h"

#include "reference.mock"


Describe(List);
BeforeEach(List) {}
AfterEach(List) {}


Ensure(List, cons_an_element_to_a_list_will_update_the_list_to_point_to_the_element) {
    Reference *element = newReference((Position){0,1,2}, UsageDefined, NULL);
    Reference *list = newReference((Position){0,1,2}, UsageDefined, NULL);
    Reference *initial_list = list;

    LIST_CONS(element, list);

    assert_that(list, is_equal_to(element));
    assert_that(list->next, is_equal_to(initial_list));
    assert_that(element->next, is_equal_to(initial_list));
}
