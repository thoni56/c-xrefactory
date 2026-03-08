include ../Makefile.server

# Test that navigation finds the correct reference when lines have shifted
# in the editor buffer (preload differs from disk file).

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create . > /dev/null
	sleep 2
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
