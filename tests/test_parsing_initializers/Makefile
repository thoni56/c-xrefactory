include ../Makefile.server

# Verifies cxref captures all references of enum value E0 used in
# designated initializers (.c[E0].a, [E0][E1], etc.). Uses cxref_extract
# (same mechanism as cxref_reader, different output focus) to dump just
# the references of E0 from the snapshot.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_extract .c-xref/db E0 | $(NORMALIZE) > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
