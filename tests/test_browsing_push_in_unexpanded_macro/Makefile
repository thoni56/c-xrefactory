# Browsing from inside an unexpanded macro body
#
# The cursor is on calculate_area inside the body of GET_AREA in macro.h, which
# is not a symbol until the macro is expanded. The server looks for a CU that
# expands the macro (findMacroExpansionFile), parses it, and resolves the symbol
# there.
#
# The requests use project discovery on purpose. With the legacy -p/-xrefrc the
# project config schedules every file in the directory, and
# prepareInputFileForRequest then prepares the lowest file number - a hash of
# the path - so source.c could win over the request file macro.h. That made this
# test fail on macOS and pass on Linux. The server bug is still there; it is
# pinned by the suspended Cgreen test
# Server -> prepares_the_request_file_when_others_are_also_scheduled.
#
# This is a legacy test: it needs the batch -create, because only XrefMode
# records the macro's base-file reference that findMacroExpansionFile looks for.
# The server alone answers "No reference" - see the suspended
# test_browsing_push_in_unexpanded_macro_without_create. When -create becomes
# priming rather than a requirement, this test goes with it.

include ../Makefile.server

$(TEST):
	$(CXREF_PROGRAM) -create > output.tmp
	@$(SERVER_DRIVER) commands.input $(EXTRA) >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
