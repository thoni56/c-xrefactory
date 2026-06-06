include ../Makefile.server

# Verifies the parser surfaces a "Can't open file" warning when #include
# references a non-existent file. ServerMode deliberately diverts this
# warning from the -xrefactory-II protocol stream to the log channel
# (see yylex.c:740 — `if (options.mode != ServerMode) warningMessage(...)
# else log_warn(...)`). To capture it we pass -warnings on the CXREF
# startup line (raises console log level to LOG_WARN — see main.c:42)
# and grep run.stderr instead of run.stdout.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep "Can't open file 'somefile'" run.stderr; true) \
	    | sed 's| [a-zA-Z_][a-zA-Z_]*\.[ch]:[0-9][0-9]*: ||' \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
