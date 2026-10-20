include ../Makefile.server

# Verifies that a missing closing parenthesis after __has_include,
# __has_include_next or defined() in an #if is reported at the lexem there.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>.*\(__has_include\|defined\).*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
