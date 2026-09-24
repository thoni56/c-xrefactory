# -get must answer a request that carries no input file
#
# The client asks for __PROJECT_ROOT with the bare option and no file - that is
# what c-xref-get-env sends. olcxProcessGetRequest (src/cxref.c) then runs the
# value through expandPredefinedSpecialVariables_static, whose every expansion
# is derived from the input file, and which used to reach for the file name
# helpers whether or not there was a file. With none, the server died on a null
# pointer, the client silently restarted it, and Project -> Edit Config and
# Remove References and Restart Server both quietly killed the server each time
# they were used.

include ../Makefile.server

$(TEST):
	@-rm -rf .c-xref/db CXrefs
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS = -ex 'br olcxProcessGetRequest' -ex 'continue'
