include ../Makefile.server

# Scenario: every includer of a header is found, however many there are
#
# Given 300 CUs that, like main.c, include h.h, none of them parsed
# When shared is pushed on in main.c twice
# Then the first push analyses all 300 files, and the second nothing

SIBLINGS = $(shell seq -w 1 300)

$(TEST): .c-xrefrc
	@rm -rf .c-xref sibling*.c
	@for n in $(SIBLINGS) ; do printf '#include "h.h"\nint s$$n(void) { return shared; }\n' > sibling$$n.c ; done
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -E "^-browse-push|<progress>" output.tmp | sed "s+$(CURDIR)+CURDIR+g" | awk '/<progress>/ { if (!inProgress) print; inProgress = 1; next } { inProgress = 0; print }' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
