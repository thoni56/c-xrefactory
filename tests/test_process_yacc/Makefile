# A reference from a .y file to an extern function in an included header
#
# The control for test_yacc_included_enum_reference: an extern function is the
# storage class that keeps its plain link name when parsed at nesting level > 0,
# where that test's enum constant is the case that did not.
#
# Establishing the project parses file2.c as the request file and file.y through
# entry refresh pass 3, both including file2.h. The exit writes the snapshot the
# dump reads. y.tab.c goes first: byacc is no longer run here, and a copy left
# by an older run would be parsed as one more C file in the project.

include ../Makefile.server

DUMP = ../../utils/cxref_reader

$(TEST): .c-xrefrc
	@rm -rf .c-xref dump.tmp output.tmp y.tab.c
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(DUMP) .c-xref/db > dump.tmp
	@# func is one symbol, so the call from file.y is under it, at 9:17
	@awk '/^[^ ]/{ inside = ($$0 == "func") } inside && /file\.y:9:17/ { found = 1 } END { exit !found }' dump.tmp ; \
	if [ $$? -ne 0 ] ; then \
		$(FAIL) ; \
		echo "No call of func from file.y:9:17 under the plain 'func' symbol. The dump has:" ; \
		awk '/^[^ ]/{ inside = ($$0 ~ /(^|!)func$$/) } inside' dump.tmp ; \
		exit 1 ; \
	else \
		$(OK) ; \
		exit 0 ; \
	fi

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
