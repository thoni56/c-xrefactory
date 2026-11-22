include ../Makefile.server

# Scenario: a changed config is reloaded once, not on every request after the change
#
# Given a project of a.c and b.c, analysed by a search
# When the config changes, and the search is made twice
# Then the first search after the change analyses both files again, and the second nothing

$(TEST):
	@rm -rf .c-xref .c-xrefrc
	@cp c-xrefrc.tpl .c-xrefrc
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -E "^-search|<progress>" output.tmp | sed "s+$(CURDIR)+CURDIR+g" | awk '/<progress>/ { if (!inProgress) print; inProgress = 1; next } { inProgress = 0; print }' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
