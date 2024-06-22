# The new coveralls app will automatically collect .gcov files
coveralls report

# Move "stable" tag if on main
./.travis_merge_to_stable_maybe.sh
