include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc -create -log=log -o out

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@echo "Tests in '$(TEST)': $(shell tput setaf 3)No tests, just executing$(shell tput sgr0)"
	@exit 0

gdb:
	gdb --args $(COMMAND)
