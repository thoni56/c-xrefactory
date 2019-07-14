# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc single_int*.c -o $(TEST).log

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@../../utils/cxref_reader.py > output
	@$(call CHECK_FOR_CONTENT,expected1) ; \
	e=$$? ; \
	if [ $$e -ne 0 ] ; then \
		echo "Failed: output did not contain expected1" ; \
		exit 1 ; \
	fi
	@$(call CHECK_FOR_CONTENT,expected2) ; \
	e=$$? ; \
	if [ $$e -ne 0 ] ; then \
		echo "Failed: output did not contain expected2" ; \
		exit 1 ; \
	fi
	@echo "Ok"
	@exit 0

gdb:
	gdb --args $(COMMAND)
