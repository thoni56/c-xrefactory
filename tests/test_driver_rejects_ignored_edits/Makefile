# The server driver refuses a client that ignores the server's edits
#
# After an answer that edits source.c, a real client holds the edits: the
# buffer is modified, so the next request preloads it, or it was saved, so
# the file on disk changed. This command file does neither before the -push,
# which no client can do, so the driver stops and says so.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer driver.err
	@-$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2> driver.err; echo "exit=$$?" >> output.tmp
	@cat driver.err >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
