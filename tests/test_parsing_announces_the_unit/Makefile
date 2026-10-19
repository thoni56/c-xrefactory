include ../Makefile.server

# Verifies that a parse that reports announces its compilation unit as an
# <analysing> record, also when it finds nothing. The client groups the
# messages that follow under it, and replaces the group when the unit is
# parsed again, so a clean parse is what removes the old messages.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<analysing [^>]*>[^<]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
