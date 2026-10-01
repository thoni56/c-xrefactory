include ../Makefile.server

$(TEST):
	@# Test: locked on the test directory, a file under projectB/, which has a
	@# .c-xrefrc of its own, belongs to another project and is refused
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
