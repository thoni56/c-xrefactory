include ../Makefile.server

# Scenario: Navigation from cold start finds all CUs sharing a header
#
# Given NO initial project scan has been run
# And ins.c and cla.c both include atr_x.h
# And atr_x.h declares analyzeAttributes()
# When the user PUSHes on analyzeAttributes in ins.c
# And NEXTs through all references
# Then navigation should visit ins.c, cla.c, AND atr_x.h
#
# Reproduces a real-world bug where the second CU was not discovered.

$(TEST):
# No -create: the server must discover and parse CUs on-demand
	@-rm -rf .c-xref 2>&1 > /dev/null
	@$(SERVER_DRIVER) $(EXTRA) commands.input > output.tmp
	@$(NORMALIZE) output.tmp > output
	$(VERIFY)

trace: EXTRA = --extra '-debug -log=trace'
trace: $(TEST)

GDB_COMMANDS =
