include ../Makefile.server

# Verifies an unresolved #include reaches the client as a <warning>
# protocol tag. This is what makes a misconfigured -I visible: without it
# navigation silently misses every symbol the header would have defined.
#
# The request asks for errors (-errors on the CXREF startup line), so the
# warning is wanted. `options.noErrors` keeps background reparses quiet on
# its own (referencerefresh.c), so asking per session is enough.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer run.stdout run.stderr
	-@$(SERVER_DRIVER) commands.input > run.stdout 2> run.stderr
	@(grep -o "<warning [^>]*>Can't open file somefile" run.stdout; true) \
	    | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
