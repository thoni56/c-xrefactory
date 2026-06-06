include ../Makefile.server

# Verifies the parser emits specific protocol errors/warnings when
# source.c uses #include MACRO_NAME patterns:
#   - #include <stdio.h> via macro: "Include <> after macro expansion not
#     yet implemented" warning + 2 "Syntax error" tokens
#   - #include "file.h" via macro on a missing file: "Can't open file"
#     warning
# ServerMode often suppresses parse-level tags during the parse-all
# sweep; we use -push (per-request parse) to make the errors attach to
# that request's protocol output. Greps for the four expected text
# fragments rather than diffing the full protocol stream, so the test
# stays robust to other protocol changes.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -F "Include <> after macro expansion" run.stdout; \
	  grep -F "Syntax error on: ." run.stdout; \
	  grep -F "Syntax error on: >" run.stdout) | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
