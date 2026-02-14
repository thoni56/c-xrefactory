include ../Makefile.server

# This test tries to describe what should happen on PUSH and an editor buffer is modified
# Current implementation cannot handle that so this test is suspended

$(TEST):
	touch source.preload # ensure the preloaded buffer has a more recent timestamp
	rm -rf CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
