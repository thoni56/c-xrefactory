include ../Makefile.server

# Scenario: a held CU that has been built is not built again while nothing changes
#
# Given a.c and b.c both include h.h, and a.c is held in an editor with edited content
# When shared is pushed on in b.c twice, with nothing changed in between
# Then the first push analyses a.c, and the second analyses nothing

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -E "^-browse-push|<progress>" output.tmp | sed "s+$(CURDIR)+CURDIR+g" | awk '/<progress>/ { if (!inProgress) print; inProgress = 1; next } { inProgress = 0; print }' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
