include ../Makefile.server

# Scenario: a config that still sets -refs and -refnum gets one warning
#
# Given a .c-xrefrc that sets -refs and -refnum
# When the client asks for the project and then pushes
# Then the -getproject answer warns that they are ignored
# And the -push answer does not warn again
# And the snapshot is the single .c-xref/db file

$(TEST):
	@rm -rf .c-xref elsewhere
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@ls .c-xref/db >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
