include ../Makefile.server

# Scenario: a CU is not built again for a header that its own build found
#
# Given a.c and b.c both include h.h, and a.c also includes p.h as <p.h>, which the scan
#   does not follow
# When shared is pushed on in b.c, which builds a.c, whose parse reads p.h
# And shared is pushed on again, with nothing changed in between
# Then the first push analyses a.c, and the second analyses nothing, since p.h is what
#   the build read

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@-$(SERVER_DRIVER) commands.input --show-progress --extra=-progress-unthrottled $(EXTRA) > output.tmp 2>&1
	@grep -E "^-browse-push|<progress>" output.tmp | sed "s+$(CURDIR)+CURDIR+g" | awk '/<progress>/ { if (!inProgress) print; inProgress = 1; next } { inProgress = 0; print }' > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
