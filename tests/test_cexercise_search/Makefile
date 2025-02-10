# Automated tests for exercising completion functionality

include ../Makefile.boilerplate


$(TEST): CXrefs
	@$(EDIT_SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-trace\'
trace: $(TEST)

CXrefs:
	@$(COMMAND) test_source -create -o $(TEST).log
