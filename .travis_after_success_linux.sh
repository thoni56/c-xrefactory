# Coverage for tests to Coveralls, but only for linux
lcov -q -d . -c -o coverage.info
lcov -q --remove coverage.info '*.mock' '*.tab.c' '/usr/*' -o coverage.info
coveralls-lcov coverage.info
# Move "stable" tag if on master
./.travis.merge_to_stable_maybe.sh
