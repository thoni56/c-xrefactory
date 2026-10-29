#include <cgreen/cgreen.h>

#include "type.h"

#include "commons.mock"


Describe(Type);
BeforeEach(Type) {
    initTypeModifications();
}
AfterEach(Type) {}

Ensure(Type, combines_long_with_long_into_long_long) {
    assert_that(cTypeSpelling(combinedType(TmodLong, TmodLong)), is_equal_to_string("long long"));
}

Ensure(Type, combines_long_long_with_unsigned_into_unsigned_long_long) {
    Type longLong = combinedType(TmodLong, TmodLong);
    assert_that(cTypeSpelling(combinedType(longLong, TmodUnsigned)), is_equal_to_string("unsigned long long"));
}

Ensure(Type, combines_unsigned_long_with_long_into_unsigned_long_long) {
    Type unsignedLong = combinedType(TmodUnsigned, TmodLong);
    assert_that(cTypeSpelling(combinedType(unsignedLong, TmodLong)), is_equal_to_string("unsigned long long"));
}
