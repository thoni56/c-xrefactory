include ../Makefile.server

$(TEST): include.h
	@-rm -rf include2.h* .c-xref
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
	@-rm -rf .c-xref include2.h*

GDB_COMMANDS =
