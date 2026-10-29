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

Ensure(Type, combines_int_with_long_long_into_long_long_int) {
    assert_that(cTypeSpelling(combinedType(TypeInt, TmodLongLong)), is_equal_to_string("long long int"));
}

Ensure(Type, combines_int_with_unsigned_long_long_into_unsigned_long_long_int) {
    assert_that(cTypeSpelling(combinedType(TypeInt, TmodLongLongUnsigned)), is_equal_to_string("unsigned long long int"));
}

Ensure(Type, combines_long_long_int_with_unsigned_into_unsigned_long_long_int) {
    assert_that(cTypeSpelling(combinedType(TypeLongLongInt, TmodUnsigned)), is_equal_to_string("unsigned long long int"));
}

Ensure(Type, combines_long_long_with_signed_into_signed_long_long) {
    Type longLong = combinedType(TmodLong, TmodLong);
    assert_that(cTypeSpelling(combinedType(longLong, TmodSigned)), is_equal_to_string("signed long long"));
}

Ensure(Type, combines_signed_long_with_long_into_signed_long_long) {
    Type signedLong = combinedType(TmodSigned, TmodLong);
    assert_that(cTypeSpelling(combinedType(signedLong, TmodLong)), is_equal_to_string("signed long long"));
}

Ensure(Type, combines_int_with_signed_long_long_into_signed_long_long_int) {
    assert_that(cTypeSpelling(combinedType(TypeInt, TmodLongLongSigned)), is_equal_to_string("signed long long int"));
}

Ensure(Type, combines_long_long_int_with_signed_into_signed_long_long_int) {
    assert_that(cTypeSpelling(combinedType(TypeLongLongInt, TmodSigned)), is_equal_to_string("signed long long int"));
}
