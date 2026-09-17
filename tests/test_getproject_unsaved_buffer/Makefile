include ../Makefile.server

# Scenario: the user has a new file open in the editor and has not saved it
# yet, so it is not on disk. The client sends getproject for it, and the server
# should accept it and answer with the discovered project, the same answer it
# gives for a file in that directory that does exist.

$(TEST):
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
