include ../Makefile.boilerplate

# There should be 27 references to vApplClass
# so one line with the reference item gives 28
# in total

ARGUMENTS = -xrefactory-II -create -errors -o output.tmp src

$(TEST): c_xrefrc_for_single_cxref_file
	-rm CXrefs
	$(COMMAND) | grep -v progress > log
	../../utils/cxref_reader.py CXrefs > tmp
	./extract_vApplClass.sh tmp | wc -l | tr -d ' ' > output
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
	$(COMMAND) | grep -v progress > /dev/null
	awk '/vApplClass/ {print $$3}' CXrefs > expected
