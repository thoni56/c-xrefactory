include ../Makefile.server

# Verifies that the parser reports, as <log> records, an #include of a
# macro whose body is <stdio.h>: "#include <...> from a macro is not
# supported", and 2 "Syntax error" from the tokens left after it
# (backlog #60). Greps for the expected text fragments rather than
# diffing the full protocol stream, so the test stays robust to other
# protocol changes.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -E -o "<log [^>]*>(Include <>|#include <...>)[^<]*" run.stdout | head -1; \
	  grep -F "Syntax error on: ." run.stdout; \
	  grep -F "Syntax error on: >" run.stdout) | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
