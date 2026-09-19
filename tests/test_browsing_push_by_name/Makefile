include ../Makefile.server

# Scenario: push by name on a cold start. No -create, so nothing is indexed
# and the server has to parse the project itself before it can find the name.

$(TEST):
	@rm -rf .c-xref out output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

GDB_COMMANDS =
