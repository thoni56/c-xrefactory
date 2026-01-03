include ../Makefile.cli

$(TEST): .c-xrefrc
	@$(COMMAND) -create -o $(TEST).log
	@../../utils/cxref_reader | grep "var_" > output
	$(VERIFY)
