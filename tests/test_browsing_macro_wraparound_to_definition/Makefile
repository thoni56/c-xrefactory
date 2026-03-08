include ../Makefile.server

# Test that navigation wraps around to the DEFINITION, not just the first
# reference in filename order. The definition is in zzz_define.h which comes
# LAST alphabetically, so wrap-around should skip to it, not aaa_usage.c.

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create > /dev/null
	sleep 2
# Create preload files for all source files (simulating Emacs buffers)
	cp zzz_define.h zzz_define.preload
	cp aaa_usage.c aaa_usage.preload
	cp bbb_usage.c bbb_usage.preload
	cp ccc_usage.c ccc_usage.preload
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
