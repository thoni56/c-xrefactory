include ../Makefile.server

# Scenario: siblings above the cap are counted once, however many headers they share
#
# Given 130 CUs that, like main.c, include both a.h and b.h, none of them parsed
# When a is pushed on in main.c
# Then the progress says 130 unparsed siblings, of which 128 are parsed

SIBLINGS = $(shell seq -w 1 130)

$(TEST): .c-xrefrc
	@rm -rf .c-xref sibling*.c
	@for n in $(SIBLINGS) ; do printf '#include "a.h"\n#include "b.h"\nint s$$n(void) { return a; }\n' > sibling$$n.c ; done
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -m1 '<progress>' output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
