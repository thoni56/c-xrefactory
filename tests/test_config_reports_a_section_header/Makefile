include ../Makefile.server

# Given a .c-xrefrc that starts with a section header
# When the server loads it
# Then the header reaches the client as a <log> at its position, since a config
# has no sections. A cold start loads the config twice (#3), hence the two pairs.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -oE "<(analysing|log) [^>]*>[^<]*c-xrefrc[^<]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
