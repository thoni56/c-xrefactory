#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "contenttoken.h"

#include "fileio.mock"


Describe(ContentToken);
BeforeEach(ContentToken) {}
AfterEach(ContentToken) {}

Ensure(ContentToken, tokens_with_the_same_time_and_size_are_the_same) {
    ContentToken a = makeContentToken(makeFileTimestamp(1000, 500), 42);
    ContentToken b = makeContentToken(makeFileTimestamp(1000, 500), 42);
    assert_that(contentTokensAreSame(a, b), is_true);
}

Ensure(ContentToken, tokens_with_different_times_are_not_the_same) {
    ContentToken a = makeContentToken(makeFileTimestamp(1000, 500), 42);
    ContentToken b = makeContentToken(makeFileTimestamp(1000, 501), 42);
    assert_that(contentTokensAreSame(a, b), is_false);
}

Ensure(ContentToken, an_older_time_is_not_the_same_either) {
    ContentToken seen = makeContentToken(makeFileTimestamp(2000, 0), 42);
    ContentToken restored = makeContentToken(makeFileTimestamp(1000, 0), 42);
    assert_that(contentTokensAreSame(restored, seen), is_false);
}

Ensure(ContentToken, tokens_with_different_sizes_are_not_the_same) {
    ContentToken a = makeContentToken(makeFileTimestamp(1000, 500), 42);
    ContentToken b = makeContentToken(makeFileTimestamp(1000, 500), 43);
    assert_that(contentTokensAreSame(a, b), is_false);
}

Ensure(ContentToken, missing_is_the_same_as_missing) {
    assert_that(contentTokensAreSame(missingContentToken(), missingContentToken()), is_true);
}

Ensure(ContentToken, missing_is_not_the_same_as_a_file) {
    ContentToken file = makeContentToken(makeFileTimestamp(1000, 500), 42);
    assert_that(contentTokensAreSame(missingContentToken(), file), is_false);
    assert_that(contentTokensAreSame(file, missingContentToken()), is_false);
}

Ensure(ContentToken, an_empty_file_at_time_zero_is_not_missing) {
    ContentToken empty = makeContentToken(NULL_TIMESTAMP, 0);
    assert_that(contentTokenIsMissing(empty), is_false);
    assert_that(contentTokensAreSame(empty, missingContentToken()), is_false);
}

Ensure(ContentToken, missing_is_missing) {
    assert_that(contentTokenIsMissing(missingContentToken()), is_true);
}

Ensure(ContentToken, a_file_that_cannot_be_stated_is_missing) {
    expect(fileStatus, when(path, is_equal_to_string("gone.h")), will_return(-1));
    assert_that(contentTokenIsMissing(contentTokenOf("gone.h")), is_true);
}

Ensure(ContentToken, a_file_has_the_time_and_size_of_one_stat) {
    struct stat st = {0};
#ifdef __APPLE__
    st.st_mtimespec = makeFileTimestamp(1000, 500);
#else
    st.st_mtim = makeFileTimestamp(1000, 500);
#endif
    st.st_size = 42;
    expect(fileStatus, when(path, is_equal_to_string("header.h")),
           will_set_contents_of_parameter(statbuf, &st, sizeof(st)), will_return(0));

    ContentToken token = contentTokenOf("header.h");

    assert_that(contentTokensAreSame(token, makeContentToken(makeFileTimestamp(1000, 500), 42)), is_true);
}

/* The value is kept in the snapshot, so it must not change between runs,
 * builds or platforms */
Ensure(ContentToken, a_known_token_has_a_known_value) {
    ContentToken token = makeContentToken(makeFileTimestamp(1000, 500), 42);
    assert_that(contentTokenAsInt(token), is_equal_to(1426731049));
}

Ensure(ContentToken, missing_is_kept_as_zero) {
    assert_that(contentTokenAsInt(missingContentToken()), is_equal_to(0));
    assert_that(contentTokenIsMissing(contentTokenFromInt(0)), is_true);
}

Ensure(ContentToken, a_token_read_back_is_the_same) {
    ContentToken token = makeContentToken(makeFileTimestamp(1000, 500), 42);
    assert_that(contentTokensAreSame(contentTokenFromInt(contentTokenAsInt(token)), token), is_true);
}
