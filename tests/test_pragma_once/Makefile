include ../Makefile.boilerplate

$(TEST): .c-xrefrc
	@$(COMMAND) -create -o $(TEST).log
	@../../utils/cxref_reader | grep "var_" > output
	$(VERIFY)
