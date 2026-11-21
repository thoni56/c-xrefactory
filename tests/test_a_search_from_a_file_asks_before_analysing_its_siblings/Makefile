include ../Makefile.server

# Scenario: a search from a file asks before analysing its siblings
#
# Given 130 CUs that, like main.c, include h.h, none of them parsed
# When a search is made from main.c
# Then it asks before analysing any of them, since a search is over the whole project

SIBLINGS = $(shell seq -w 1 130)

$(TEST): .c-xrefrc
	@rm -rf .c-xref sibling*.c
	@for n in $(SIBLINGS) ; do printf '#include "h.h"\nint s$$n(void) { return shared; }\n' > sibling$$n.c ; done
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@grep -E "wait-confirmation|<error|FATAL" output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
