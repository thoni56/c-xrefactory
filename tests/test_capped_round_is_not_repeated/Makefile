include ../Makefile.server

# Scenario: a capped build is finished by the next request, not repeated
#
# Given 130 CUs that, like main.c, include both a.h and b.h, none of them parsed
# When a is pushed on in main.c three times
# Then the first push analyses 128 of 130 files, the second the 2 left over,
#   and the third nothing

SIBLINGS = $(shell seq -w 1 130)

$(TEST): .c-xrefrc
	@rm -rf .c-xref sibling*.c
	@for n in $(SIBLINGS) ; do printf '#include "a.h"\n#include "b.h"\nint s$$n(void) { return a; }\n' > sibling$$n.c ; done
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -E "^-browse-push|<progress>" output.tmp | sed "s+$(CURDIR)+CURDIR+g" | awk '/<progress>/ { if (!inProgress) print; inProgress = 1; next } { inProgress = 0; print }' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
