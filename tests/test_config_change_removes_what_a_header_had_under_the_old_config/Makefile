include ../Makefile.server

# Scenario: a config change removes what a header had under the old config
#
# Given h.h declares feature_only only when FEATURE is defined, and a.c includes h.h
# And the config defines FEATURE, so a search finds feature_only in h.h
# When the config no longer defines FEATURE
# Then the search finds nothing, since a config change is a change of every file

$(TEST):
	rm -rf .c-xrefrc
	cp c-xrefrc.tpl .c-xrefrc
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
