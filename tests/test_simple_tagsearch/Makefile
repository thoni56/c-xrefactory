include ../Makefile.server


$(TEST): CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = \'-debug\'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF) -create test_source -o $(TEST).log
