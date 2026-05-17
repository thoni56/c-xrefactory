PREPARE = prepare

include ../Makefile.server


$(TEST): prepare
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

prepare:
	@-rm -rf include2.h* .c-xref
	@test -f include.h || git checkout include.h
	@touch include.h

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
