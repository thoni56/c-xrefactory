include ../Makefile.boilerplate


ARGUMENTS = -refnum=1 single_int*.c -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(NO_TESTS)
	@exit 0
