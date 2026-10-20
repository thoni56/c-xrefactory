include ../Makefile.server

# Verifies that a recovery from a syntax error goes to the debug log, not to
# the client. The syntax error itself is still sent.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr log
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>[^<]*\|<error [^>]*>[^<]*" run.stdout; \
	  grep -o ": recovery$$" log; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
