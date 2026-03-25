include ../Makefile.server

$(TEST): include.h
	@-rm -rf include2.h* .c-xref CXrefs
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

include.h:
	-rm include2.h*
	touch include.h

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

clean: clean-test-artifacts
clean-test-artifacts:
	@-rm -rf .c-xref CXrefs include2.h*

GDB_COMMANDS =
