# Automated tests for exercising completion server

include ../Makefile.boilerplate


$(TEST): CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF) test_source -create -o $(TEST).log
