# Scenario: a header follows a let-go of a header it includes
#
# Given a.c includes mid.h, which includes leaf.h
# And leaf.h is held with content that defines FEATURE, so mid.h declares feature_only
# When the next push in a.c no longer holds leaf.h, which on disk does not define it
# Then feature_only is no longer in mid.h
#
# A change of a file is a change of every file that includes it, also when it is let go
# (ADR-0035).

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp leaf.h.without_feature leaf.h
	@touch -t 200001010000 a.c mid.h leaf.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
