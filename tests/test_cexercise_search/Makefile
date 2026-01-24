# Automated tests for exercising completion server

include ../Makefile.server


$(TEST): CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

CXrefs:
	@$(COMMAND) test_source -create -o $(TEST).log
