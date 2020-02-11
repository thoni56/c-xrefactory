# Automated tests with the primary purpose of adding coverage
# to the yaccparser
#
# Very simple verification of some symbols, actually that 'grep' trick
# (it only looks for the same lines in the output as in the expected
# in any order) isn't very foolproof so we should write a utility for
# checking that a symbol reference is there

include ../Makefile.boilerplate

COMMAND = ../../src/c-xref -xrefrc .c-xrefrc simple_grammar.y -o $(TEST).log

# Alternate way to look for content in a file
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
