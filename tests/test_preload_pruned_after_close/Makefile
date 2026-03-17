include ../Makefile.server

# Scenario: Server drops ghost preloaded buffer when client stops preloading
#
# Given a file is preloaded to the server (modified in editor)
# And the server parses it using the preloaded content
# When the next request does NOT include the preload (user saved/closed)
# And the disk file has different content
# Then the server should read from disk, not the stale preloaded buffer

$(TEST):
	@-rm -r .c-xref/db
	@cp source.c.orig source.c
	@cp source.c.orig source.preload
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	@cp source.c.orig source.c
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
