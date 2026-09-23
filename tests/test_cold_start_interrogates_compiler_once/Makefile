# A cold start asks the compiler about itself once
#
# The compiler is a script that logs how it was called and then runs gcc.
# -getproject asks it for include paths and predefined macros and saves a
# checkpoint. The -push that follows should restore that checkpoint instead
# of asking again.

include ../Makefile.server

$(TEST): .c-xrefrc
	@rm -rf .c-xref output output.tmp server-buffer compiler.log
	@$(SERVER_DRIVER) commands.input $(EXTRA) > output.tmp
	@cat compiler.log >> output.tmp 2>/dev/null || echo "the compiler was never called" >> output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra "-debug -log=trace"
trace: $(TEST)

GDB_COMMANDS =
