include ../Makefile.server

# Verifies a syntax error says the column of the lexem it is on, counted
# from 1 as compilers do, so next-error goes to the lexem and not to the
# first non-blank on the line.

$(TEST):
	@rm -rf output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<log [^>]*>[^<]*Syntax error on: [a-z]*" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
