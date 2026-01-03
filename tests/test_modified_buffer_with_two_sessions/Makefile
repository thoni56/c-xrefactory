include ../Makefile.server

# This test describes how navigation should move along with modified editor buffers
# Current implementation cannot do that, so this test is suspended

$(TEST):
	touch source.preload # ensure the preloaded buffer has a more recent timestamp
	$(CXREF) -create > /dev/null 2>&1
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=log"
trace: $(TEST)

GDB_COMMANDS =
