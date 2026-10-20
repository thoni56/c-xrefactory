include ../Makefile.server

# Verifies that a file that ends inside the arguments of a macro call, or
# right after its '(', is reported where the macro is used.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>.*macro call.*\|<error [^>]*>.*\|<goto>" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
