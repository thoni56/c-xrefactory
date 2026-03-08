include ../Makefile.server

# Test that navigation visits the definition after wrap-around, even when
# each file is preloaded (triggering stale refresh) as we navigate.

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create > /dev/null
	sleep 2
# Create preload files for all source files (simulating Emacs buffers)
	cp common.h common.preload
	cp source1.c source1.preload
	cp source2.c source2.preload
	cp source3.c source3.preload
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
