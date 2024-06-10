include ../Makefile.boilerplate


ARGUMENTS = -xrefrc -refnum=1 .c-xrefrc single_int*.c -o log

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@$(NO_TESTS)
	@exit 0

clean:
	rm -rf CXrefs .c-xrefrc
