# References follow a changed source, where "changed" is an unsaved editor buffer
#
# source.preload is source.c with a line inserted and an extra global, the way an
# Emacs buffer differs from the file on disk. Phase 1 browses the file as it is on
# disk. Phase 2 re-runs with -preload and browses the buffer's shifted positions.
#
# No -create prime, the server parses on demand. .c-xref has to go before each run.
# The snapshot the previous run left holds the preload positions, because saving
# records unsaved buffer state, and reading it back makes those positions look like
# disk truth.

include ../Makefile.server

$(TEST):
	-rm -rf .c-xref
	touch source.preload
	$(SERVER_DRIVER) $(SERVER_EXTRA) commands.input > output.tmp
	$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: SERVER_EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

db:
	$(CXREF) -xref -create
	../../utils/cxref_reader

fresh:
	-rm -rf CXrefs
	$(CXREF) -xref -create
	../../utils/cxref_reader

fastupdate:
	$(CXREF) -xref -fastupdate -preload source.c source.preload
	../../utils/cxref_reader
