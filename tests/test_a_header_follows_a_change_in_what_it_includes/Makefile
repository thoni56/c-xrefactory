# Scenario: a header follows a change in a header it includes
#
# Given a.c includes mid.h, which includes leaf.h
# And leaf.h defines FEATURE, so mid.h declares feature_only
# When leaf.h is rewritten on disk without FEATURE
# And a push in a.c builds it again
# Then feature_only is no longer in mid.h
#
# A change of a file is a change of every file that includes it (ADR-0035).

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer
	@cp leaf.h.with_feature leaf.h
	@touch -t 200001010000 a.c mid.h leaf.h
	-@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@../../utils/cxref_reader .c-xref/db > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
