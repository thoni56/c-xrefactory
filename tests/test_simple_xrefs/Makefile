include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefrc .c-xrefrc single_int*.c -o log
COMMAND = $(CXREF) $(ARGUMENTS)

CHECK_FOR_CONTENT = python -c "content=open('$(1)').read(); target=open('output').read(); exit(0 if content in target else 1);"

$(TEST):
	@rm -rf output $@.log CXrefs Xrefs
	@$(COMMAND)
	@../../utils/cxref_reader.py > output
	@while read -r line ; do \
		grep -q $$line output ; \
		ecode=$$? ; \
		if [ $$ecode -ne 0 ] ; then \
			echo "Tests in '$(TEST)': $(shell tput setaf 1)Failed$(shell tput sgr0) - output did not contain expected: '$$line'" ; \
			exit 1 ; \
		fi ; \
	done < expected
	@$(OK)
	@exit 0
