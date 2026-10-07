include ../Makefile.server

# Scenario: a dump of something it does not know says so
#
# When a dump is asked for a selection there is none of
# Then the answer is an error naming the selection and the ones there are

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
