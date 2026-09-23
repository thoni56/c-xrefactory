# A reference from a .y file to an enum constant in an included header
#
# test_process_yacc is the control: it does the same thing with an extern
# function, which is the one storage class that keeps its plain link name
# when parsed at nesting level > 0.
#
# Establishing the project parses both CUs on its own - file2.c as the request
# file, file.y through entry refresh pass 3, since both include file2.h - so
# the PUSH is not what makes the references exist. It is here because asking
# for the symbol is what a user does, and the test should break if pushing on
# an enum constant stops working. The exit writes the snapshot the dump reads.

include ../Makefile.server

DUMP = ../../utils/cxref_reader

$(TEST): .c-xrefrc
	@rm -rf .c-xref dump.tmp output.tmp
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(DUMP) .c-xref/db > dump.tmp
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

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
