include ../Makefile.boilerplate

CXREF = ../../src/c-xref
ARGUMENTS = -xrefactory-II -create -errors -xrefrc .c-xrefrc -p $(CURDIR) -o output.tmp src

$(TEST): c_xrefrc_for_single_cxref_file
	-rm CXrefs
	$(CXREF) $(ARGUMENTS) | grep -v progress > log
	 ../../utils/cxref_reader.py CXrefs | grep vApplClass | wc -l | tr -d ' ' > output
	$(VERIFY)

c_xrefrc_for_single_cxref_file: .c-xrefrc
ifeq ($(shell uname), Darwin)
	sed -i '' 's/refnum=10/refnum=1/' .c-xrefrc
else
	sed -i 's/refnum=10/refnum=1/' .c-xrefrc
endif

.PHONY: expected
expected: COMMAND = ../../src/c-xref.orig
expected:
	$(COMMAND) $(ARGUMENTS) | grep -v progress > /dev/null
	awk '/vApplClass/ {print $$3}' CXrefs > expected
