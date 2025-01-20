include ../Makefile.boilerplate


ARGUMENTS = single_int*.c -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@../../utils/cxref_reader > output
	@$(VERIFY)
