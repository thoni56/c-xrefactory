include ../Makefile.server


$(TEST): CXrefs
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	@$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

CXrefs: .c-xrefrc
	@$(CXREF_PROGRAM) -create . -o $(TEST).log
