include ../Makefile.server

$(TEST): include.h
	@-rm -f include2.h*
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

include.h:
	-rm include2.h*
	touch include.h

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
