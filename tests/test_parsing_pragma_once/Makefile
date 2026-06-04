include ../Makefile.server

# Verifies #pragma once: source.c includes include.h twice, with PART=1 then PART=2.
# Pragma once should prevent the second parse — only var_1 should appear, not var_2.

$(TEST):
	@rm -rf CXrefs .c-xref output output.tmp server-buffer
	@$(SERVER_DRIVER) commands.input > /dev/null
	@../../utils/cxref_reader .c-xref/db | grep "var_" > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
