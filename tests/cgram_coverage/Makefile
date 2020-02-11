# Automated tests for checking generated c-xreferences

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc cgram_coverage.c -o $(TEST).log

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@printf "Tests in '$@': "
	@rm -rf output $@.log CXrefs
	@$(COMMAND)
	@../../utils/cxref_reader.py > output
	@while read -r line ; do \
		grep -q $$line output ; \
		ecode=$$? ; \
		if [ $$ecode -ne 0 ] ; then \
			echo "Failed: output did not contain expected: '$$line'" ; \
			exit 1 ; \
		fi ; \
	done < expected
	@echo "Ok"
	@exit 0

gdb:
	gdb --args $(COMMAND)
