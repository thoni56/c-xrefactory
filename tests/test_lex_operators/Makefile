include ../Makefile.boilerplate


ARGUMENTS = -create operators.c -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(OK)
	@exit 0
