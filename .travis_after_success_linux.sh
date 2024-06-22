# Coverage for tests to Coveralls, but only for linux
#lcov -q -d src -c -o coverage.info
#lcov -q --remove coverage.info '*.mock' '*.tab.c' '/usr/*' -o coverage.info

# coveralls -r src -b src -E '^.*\.mock$' -E '^.*\.tab\.c$'

# The new coveralls app will automatically collect .gcov files
coveralls report

# Move "stable" tag if on main
./.travis_merge_to_stable_maybe.sh
