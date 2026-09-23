# The server is left talking to nobody
#
# The command file ends while the server waits for an answer to the parse-all
# question, so the driver closes the channel without saying -exit. That is what
# an editor that died looks like from the server's side: it notices at its next
# read, and has to go away - no records written at a closed channel, no orphan
# process holding the reference table, nothing for the driver to report.
#
# The expected output therefore stops at the question. Anything after it means
# the server said something to a client that was not there; a missing exit is
# reported by the driver, whose drain is bounded.
#
# Written in 2019 as "Ensure no errors from broken pipe" (f70cc56c) and did not
# assert anything after that: the check was 'grep -q ... > output', and grep -q
# writes nothing, so an empty output was compared against an empty expected. It
# also stopped reaching the server at all once a mode became mandatory.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp 2>&1
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)
