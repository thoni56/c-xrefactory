include ../Makefile.server

# Scenario: a rename finds a symbol that only pass 1 defines
#
# Given a config with -pass1 -DPASS1 and -pass2 -DPASS2
# And only1 with a use under #ifdef PASS1
# When only1 is renamed at its declaration
# Then both its declaration and its use are replaced

$(TEST): .c-xrefrc
	@rm -rf output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input --answer-timeout=10 $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
