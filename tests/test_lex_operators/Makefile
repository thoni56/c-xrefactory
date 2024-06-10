include ../Makefile.boilerplate


ARGUMENTS = -xrefrc .c-xrefrc -create operators.c -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(OK)
	@exit 0
