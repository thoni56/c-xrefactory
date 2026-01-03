include ../Makefile.cli

# This test doesn't really test anything. I was hoping that we could get the variables a
# and b to become references in the expansion of line 3, but obviously not ...
# But it executes the code for adding command line definitions using '-D...'

ARGUMENTS = -DA=a -DB=b source.c

$(TEST):
	$(COMMAND) > output.tmp
	grep 2/[AB] CXrefs/* | wc -l | tr -d ' ' > output
	$(VERIFY)

trace: ARGUMENTS += -debug
trace: $(TEST)

EXTRA_DEBUG =
