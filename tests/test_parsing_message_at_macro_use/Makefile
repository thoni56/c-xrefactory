include ../Makefile.server

# Verifies that a syntax error in a token from a macro body is reported
# where the outermost macro is used, with a note for each macro it
# was expanded from, as clang does.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>[^<]*\(Syntax error on\|expanded from macro\)[^<]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
