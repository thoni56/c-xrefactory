include ../Makefile.server

# Verifies that an unterminated " in the .c-xrefrc reaches the client as
# a <log> under an <analysing> for the config, at the quote's position.
# It is reported once per load of the config, and again after it changes.
# A cold start loads it twice (#3), which is why the first two pairs are there.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -oE "<(analysing|log) [^>]*>[^<]*c-xrefrc[^<]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
