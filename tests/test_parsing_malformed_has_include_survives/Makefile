include ../Makefile.server

# Verifies that a malformed __has_include in an #if is reported where it
# goes wrong and does not stop the server. The <goto> shows the request
# was answered.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>.*__has_include.*\|<error [^>]*>.*\|<goto>" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
