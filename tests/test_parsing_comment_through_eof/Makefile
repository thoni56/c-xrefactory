include ../Makefile.server

# Verifies a comment still open at the end of the file reaches the client
# as a <log> protocol tag, not as a modal <warning>.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<[a-z]* [^>]*>[^<]*comment[a-z ]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
