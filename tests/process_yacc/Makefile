# Automated tests for <test case>

test_case = $(notdir $(shell pwd))

all: $(test_case)

# Note to self:
# The Xrefs file contains all references in a format that I have yet
# to completely decipher, but here are some details:
#
# A line for a file starts with an identification: '1541f'
# A line for a symbol starts with a field that includes the symbol name: '5/func'
# such a line also includes all references including file reference, line and char:
# '1541f9l17cr' means file 1541, line 9, character 17
#
# This should be possible to decipher from the source, but where...

$(test_case):
	@rm -f $@.log Xrefs
	@printf "Tests in \"$@\": "
	@bison file.y
	@../../src/c-xref *.[cy] -o $@.log
	@grep '	5/func	' Xrefs | grep '1541f9l17cr' > /dev/null ; \
	if [ $$? -ne 0 ] ; then \
		echo "Failed" ; \
		exit 1 ; \
	else \
		echo "Ok" ; \
	fi

