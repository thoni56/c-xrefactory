include ../Makefile.cli

# The server takes no file at startup: every request names its own. A bare
# word on the startup command line is refused, and c-xref exits non-zero
# instead of quietly entering it in the file table.
#
# stdin is empty, so a server that wrongly starts sees end of input and exits
# on its own instead of hanging the test. It exits 1 then too, so exit=1 does
# not tell the two apart; the message does.

$(TEST):
	@-$(CXREF_PROGRAM) -server source.c < /dev/null > output.tmp 2>&1; echo "exit=$$?" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = -debug -log=trace
trace: $(TEST)

EXTRA_DEBUG =
