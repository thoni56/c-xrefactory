include ../Makefile.server

# Test that NEXT finds references to a macro inside another macro's definition.
# INNER_MACRO is defined on line 5, and used inside OUTER_MACRO on line 8.
# PUSH on INNER_MACRO should find the definition, NEXT should find the usage
# inside OUTER_MACRO's body.

$(TEST):
	$(CXREF_PROGRAM) -p $(CURDIR) -create > /dev/null
	sleep 2
# Create preload files for source files (simulating Emacs buffers)
	cp macros.h macros.preload
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
