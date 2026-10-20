include ../Makefile.server

# Verifies an unresolved #include reaches the client as a <log>
# protocol tag. This is what makes a misconfigured -I visible: without it
# navigation silently misses every symbol the header would have defined.
#
# The server is asked for messages (-errors on the CXREF startup line), and
# then the request's own parse reports. `silenceMessages()` keeps the other
# parses quiet (referencerefresh.c).

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>[^<]*Can't open file somefile" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
