#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"
#include "memory.h"

#include "macroargumenttable.h"

#include "commons.mock"
#include "stackmemory.mock"


Describe(MacroArgumentTable);
BeforeEach(MacroArgumentTable) {
    log_set_level(LOG_ERROR);
    initCxMemory();
    allocateMacroArgumentTable(100);
}
AfterEach(MacroArgumentTable) {}

Ensure(MacroArgumentTable, returns_null_for_non_existing_element) {
    assert_that(getMacroArgument(10), is_null);
}

Ensure(MacroArgumentTable, can_return_existing_element) {
    MacroArgumentTableElement element = { .name = "element", .linkName = "element" };
    int index = addMacroArgument(&element);
    assert_that(getMacroArgument(index), is_equal_to(&element));}
