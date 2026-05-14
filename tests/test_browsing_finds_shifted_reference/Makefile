PREPARE = prepare

include ../Makefile.server

# Test that navigation finds the correct reference when lines have shifted
# in the editor buffer (preload differs from disk file).

prepare:
	@-rm -rf .c-xref server-buffer output.tmp output trace.log
	@$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
	@touch source.preload

$(TEST): prepare
	sleep 2
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
