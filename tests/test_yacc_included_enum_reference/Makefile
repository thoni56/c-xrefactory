# A reference from a .y file to an enum constant in an included header
#
# test_process_yacc is the control: it does the same thing with an extern
# function, which is the one storage class that keeps its plain link name
# when parsed at nesting level > 0.

include ../Makefile.cli

ARGUMENTS = -xref file2.c file.y
DUMP = ../../utils/cxref_reader

$(TEST): .c-xrefrc
	@rm -rf $(TEST).log CXrefs dump.tmp
	@$(COMMAND) -o $(TEST).log
	@$(DUMP) CXrefs > dump.tmp
	@# RED is one symbol, so its uses from both file2.c and file.y are under it
	@awk '/^[^ ]/{ inside = ($$0 == "RED") } inside && /file\.y:/ { found = 1 } END { exit !found }' dump.tmp ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "No use of RED from file.y under the plain 'RED' symbol. The dump has:" ; \
		awk '/^[^ ]/{ inside = ($$0 ~ /(^|!)RED$$/) } inside' dump.tmp ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi
