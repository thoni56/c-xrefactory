include ../Makefile.server

# Scenario: a config changed while no server runs is seen by the next one
#
# Given h.h declares feature_only only when FEATURE is defined, and a.c includes h.h
# And a server with a config that defines FEATURE finds feature_only in h.h, and exits
# When the config no longer defines FEATURE
# And a new server starts from the snapshot and searches again
# Then it finds nothing, since the config is a file whose content changed

$(TEST):
	rm -rf .c-xrefrc .c-xref
	cp c-xrefrc.tpl .c-xrefrc
	@$(SERVER_DRIVER) session1.input $(EXTRA) > output.tmp
	cp c-xrefrc.without_feature .c-xrefrc
	@$(SERVER_DRIVER) session2.input $(EXTRA) >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
