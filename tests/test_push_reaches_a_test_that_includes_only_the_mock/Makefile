include ../Makefile.server

# Scenario: a push in the implementation reaches the test CU that includes only the mock
#
# Given f.h declares f, f.c defines it, and f.mock includes f.h and defines it too
# And f_tests.c includes only f.mock, and calls f
# When f is pushed on in f.c, from a cold start
# Then the definition in f.mock and the call in f_tests.c are among its references

$(TEST): .c-xrefrc
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
